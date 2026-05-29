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

# Таймаут на одну симуляцию (в секундах)
TASK_TIMEOUT = 30.0
# ==========================================================

def load_optimal_parameters():
    """Считывает таблицу средних оптимальных параметров для каждого L"""
    params_dict = {}
    try:
        with open(PARAMS_CSV, mode='r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                l_val = int(row['L'])
                params_dict[l_val] = {
                    'c_att': row['c_att'],
                    'c_opp': row['c_opp'],
                    'c_clause': row['c_clause'],
                    'gamma': row['gamma']
                }
    except FileNotFoundError:
        print(f"[ERR] Сначала создайте сводный файл параметров {PARAMS_CSV}!")
    return params_dict

def run_single_task(task_args):
    filepath, laws, method, p = task_args
    filename = os.path.basename(filepath)
    
    cmd = [
        SOLVER_BIN, 
        "--file", filepath, 
        "--laws", str(laws),
        "--method", method,
        "--c_att", p['c_att'],
        "--c_opp", p['c_opp'],
        "--c_clause", p['c_clause'],
        "--gamma", p['gamma']
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True, timeout=TASK_TIMEOUT)
        output_line = result.stdout.strip()
        return f"{output_line},{method}"
    except subprocess.TimeoutExpired:
        l_extracted = filename.split('-')[0].replace('uf', '')
        return f"0,{filepath},{l_extracted},{laws},5,{TASK_TIMEOUT},{method}"
    except subprocess.CalledProcessError:
        l_extracted = filename.split('-')[0].replace('uf', '')
        return f"0,{filepath},{l_extracted},{laws},5,{TASK_TIMEOUT},{method}"

def main():
    print(f"[{datetime.now()}] Считывание агрегированных гиперпараметров ГА...")
    params_map = load_optimal_parameters()
    if not params_map:
        return

    print(f"[{datetime.now()}] Сканирование КНФ...")
    tasks = []
    
    for dim in DIMENSIONS:
        l_num = int(dim.replace("uf", ""))
        p = params_map.get(l_num)
        if not p:
            continue
            
        pattern = os.path.join(BENCH_DIR, f"{dim}-*.cnf")
        files = glob.glob(pattern)[:FILES_PER_DIM]
        
        for filepath in files:
            for laws in LAWS_MODES:
                for method in METHODS:
                    tasks.append((filepath, laws, method, p))

    total_tasks = len(tasks)
    print(f"[{datetime.now()}] Сформировано {total_tasks} проверочных задач.")

    with open(OUTPUT_CSV, mode='w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["Is_Solved", "Filepath", "Variables_L", "Laws_Count", "Attempts", "Time_Seconds", "OdeMethod"])

    print(f"[{datetime.now()}] Запуск пула валидации на серверных ядрах...\n")
    completed = 0
    
    with concurrent.futures.ProcessPoolExecutor() as executor:
        future_to_task = {executor.submit(run_single_task, task): task for task in tasks}
        
        for future in concurrent.futures.as_completed(future_to_task):
            res_line = future.result()
            if res_line:
                data_parts = res_line.split(',')
                with open(OUTPUT_CSV, mode='a', newline='') as f:
                    writer = csv.writer(f)
                    writer.writerow(data_parts)
                    
            completed += 1
            if completed % 20 == 0 or completed == total_tasks:
                print(f"Прогресс валидации: {completed}/{total_tasks} задач выполнено.")

    print(f"[{datetime.now()}] Финальный эксперимент завершен! Данные: {OUTPUT_CSV}")

if __name__ == "__main__":
    main()
