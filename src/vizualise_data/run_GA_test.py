import os
import glob
import subprocess
import csv
import pandas as pd
import concurrent.futures
from datetime import datetime

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))

SOLVER_BIN = os.path.join(ROOT_DIR, "build", "solver_nbody")
BENCH_DIR = os.path.join(ROOT_DIR, "bench", "test")
GA_CSV = os.path.join(ROOT_DIR, "genetic_research_results_2L_RK4_Greedy.csv")
ABLATION_CSV = os.path.join(ROOT_DIR, "ablation_results.csv")

METHODS = ["RK4", "RK8", "DP8Adaptive"]
LAWS = [1, 2, 3]  # 1: Гравитация, 2: +Трение, 3: +Дизъюнкты
FILES_PER_DIM = 10 # Количество файлов для усреднения и расчета погрешности

def evaluate_task(task):
    L, method, law, filepath, c_att, c_opp, c_clause, gamma = task
    
    # Конфигурируем физические законы (Ablation)
    # Закон 1: Только гравитация и отталкивание антиподов (трение = 0, дизъюнкты = 0)
    # Закон 2: + Трение вакуума (дизъюнкты = 0)
    # Закон 3: Полная модель
    active_clause = c_clause if law == 3 else 0.0
    active_gamma = gamma if law >= 2 else 0.0
    
    cmd = [
        SOLVER_BIN,
        "--file", filepath,
        "--model_type", "2L",
        "--strategy", "GreedySkip",
        "--method", method,
        "--c_att", str(c_att),
        "--c_opp", str(c_opp),
        "--c_clause", str(active_clause),
        "--gamma", str(active_gamma)
    ]
    
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        output = res.stdout.strip()
        if output and len(output.split(',')) == 4:
            is_solved, sat, total, exec_time = output.split(',')
            return (L, method, law, int(is_solved), int(sat), int(total), float(exec_time))
    except subprocess.TimeoutExpired:
        # Если интегратор взорвался (особенно Leapfrog без трения)
        return (L, method, law, 0, 0, L*4.3, 10.0) 
    except Exception:
        pass
    return None

def main():
    if not os.path.exists(GA_CSV):
        print(f"[ERR] Файл {GA_CSV} не найден! Сначала запустите генетический алгоритм.")
        return

    print(f"[{datetime.now()}] Подготовка данных из ГА...")
    ga_df = pd.read_csv(GA_CSV)
    best_params = ga_df.groupby('L').mean().reset_index()

    tasks = []
    for _, row in best_params.iterrows():
        L = int(row['L'])
        c_att, c_opp, c_clause, gamma = row['c_att'], row['c_opp'], row['c_clause'], row['gamma']
        
        pattern = os.path.join(BENCH_DIR, f"uf{L}", "*.cnf")
        if not glob.glob(pattern):
            pattern = os.path.join(BENCH_DIR, f"l{L}", "*.cnf")
            
        files = glob.glob(pattern)[:FILES_PER_DIM]
        for method in METHODS:
            for law in LAWS:
                for f in files:
                    tasks.append((L, method, law, f, c_att, c_opp, c_clause, gamma))

    print(f"[I] Сгенерировано {len(tasks)} задач для Ablation Study.")
    
    # Файл для сырых результатов
    raw_results = []
    with concurrent.futures.ProcessPoolExecutor() as executor:
        for res in executor.map(evaluate_task, tasks, chunksize=10):
            if res:
                raw_results.append(res)
                
    # Агрегируем статистику (Успех, Время + Погрешность времени)
    df = pd.DataFrame(raw_results, columns=['L', 'Method', 'Law', 'Solved', 'Sat', 'Total', 'Time'])
    df['Sat_Pct'] = (df['Sat'] / df['Total']) * 100
    
    agg_df = df.groupby(['L', 'Method', 'Law']).agg(
        Success_Rate=('Solved', lambda x: (x.sum() / len(x)) * 100),
        Time_Mean=('Time', 'mean'),
        Time_Std=('Time', 'std'),
        Sat_Pct_Mean=('Sat_Pct', 'mean')
    ).reset_index()
    
    agg_df.to_csv(ABLATION_CSV, index=False)
    print(f"[{datetime.now()}] 🏁 Данные собраны и сохранены в {ABLATION_CSV}")

if __name__ == "__main__":
    main()