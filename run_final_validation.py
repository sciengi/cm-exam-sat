import os
import glob
import subprocess
import concurrent.futures
import csv
from datetime import datetime

# ================= НАСТРОЙКИ ЭКСПЕРИМЕНТА =================
SOLVER_BIN = "./build/solver_nbody"
BENCH_DIR = "bench/test/"
PARAMS_CSV = "final_class_parameters.csv"
OUTPUT_CSV = "final_validation_results.csv"

DIMENSIONS = ["uf3", "uf5", "uf7", "uf9", "uf10", "uf12", "uf15", "uf20"]
FILES_PER_DIM = 10
LAWS_MODES = [1, 2, 3]
METHODS = ["RK4", "DP8", "Leapfrog", "DP8Adaptive"]
TASK_TIMEOUT = 40.0
# ==========================================================

def load_optimal_parameters():
    params_dict = {}
    try:
        with open(PARAMS_CSV, mode='r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                l_val = int(row['L'])
                params_dict[l_val] = {
                    'c_att': row['c_att'], 'c_opp': row['c_opp'],
                    'c_clause': row['c_clause'], 'gamma': row['gamma']
                }
    except FileNotFoundError:
        print(f"[ERR] Сначала создайте сводный файл параметров {PARAMS_CSV}!")
    return params_dict

def run_single_task(task_args):
    filepath, laws, method, p = task_args
    filename = os.path.basename(filepath)
    
    print(f"[{datetime.now().strftime('%H:%M:%S')}] [RUN] {filename} | {method} | Laws: {laws}")
    
    cmd = [
        SOLVER_BIN, "--file", filepath, "--laws", str(laws), "--method", method,
        "--c_att", p['c_att'], "--c_opp", p['c_opp'], "--c_clause", p['c_clause'], "--gamma", p['gamma']
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True, timeout=TASK_TIMEOUT)
        output_line = result.stdout.strip()
        # C++ вернул: Is_Solved,Filepath,L,Laws,Attempts,Time,Sat_Clauses,Total_Clauses
        # Дописываем в конец название метода интегрирования
        return f"{output_line},{method}"
    except Exception:
        # В случае сбоя или таймаута пишем аварийную строчку (0 выполненных дизъюнктов)
        l_extracted = filename.split('-')[0].replace('uf', '')
        return f"0,{filepath},{l_extracted},{laws},5,{TASK_TIMEOUT},0,100,{method}"

def main():
    params_map = load_optimal_parameters()
    if not params_map: return

    tasks = []
    for dim in DIMENSIONS:
        l_num = int(dim.replace("uf", ""))
        p = params_map.get(l_num)
        if not p: continue
            
        pattern = os.path.join(BENCH_DIR, f"{dim}-*.cnf")
        files = glob.glob(pattern)[:FILES_PER_DIM]
        
        for filepath in files:
            for laws in LAWS_MODES:
                for method in METHODS:
                    tasks.append((filepath, laws, method, p))

    total_tasks = len(tasks)
    print(f"Сформировано {total_tasks} задач.")

    with open(OUTPUT_CSV, mode='w', newline='') as f:
        writer = csv.writer(f)
        # Добавились новые столбцы: Sat_Clauses и Total_Clauses
        writer.writerow(["Is_Solved", "Filepath", "Variables_L", "Laws_Count", "Attempts", "Time_Seconds", "Sat_Clauses", "Total_Clauses", "OdeMethod"])

    print("Запуск пула валидации...\n")
    completed = 0
    with concurrent.futures.ProcessPoolExecutor() as executor:
        future_to_task = {executor.submit(run_single_task, task): task for task in tasks}
        for future in concurrent.futures.as_completed(future_to_task):
            res_line = future.result()
            if res_line:
                with open(OUTPUT_CSV, mode='a', newline='') as f:
                    f.write(res_line + "\n")
            completed += 1
            if completed % 20 == 0 or completed == total_tasks:
                print(f"--- СИСТЕМНЫЙ ПРОГРЕСС: {completed}/{total_tasks} задач обработано ---")

if __name__ == "__main__":
    main()