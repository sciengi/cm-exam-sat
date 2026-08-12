import os
import glob
import subprocess
import itertools
import csv
import concurrent.futures
import numpy as np
from datetime import datetime

# ================= НАСТРОЙКИ СТЕНДА GRID SEARCH =================
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))

SOLVER_BIN = os.path.join(ROOT_DIR, "build", "solver_nbody")
BENCH_DIR = os.path.join(ROOT_DIR, "bench", "test")
MASTER_CSV = os.path.join(ROOT_DIR, "grid_search_master.csv")

DIMENSIONS = ["l3", "l6", "l9", "l12", "l15", "l18"]
FILES_PER_DIM = 5  

MODELS = ["2L", "3C"]
STRATEGIES = ["HardStop", "GreedySkip"]
ODE_SOLVERS = ["RK4", "RK8", "DP8Adaptive"]

# ДИНАМИЧЕСКИЙ ГЕНЕРАТОР ПЛОТНОЙ СЕТКИ С ШАГОМ 0.1
C_ATT_RANGE = [round(x, 1) for x in np.arange(0.1, 1.5 + 0.1, 0.1)]
C_OPP_RANGE = [round(x, 1) for x in np.arange(0.5, 2.5 + 0.1, 0.1)]
C_CLAUSE_RANGE = [round(x, 1) for x in np.arange(1.0, 3.0 + 0.1, 0.1)]
GAMMA_RANGE = [round(x, 1) for x in np.arange(0.2, 2.2 + 0.1, 0.1)]
# =================================================================

def evaluate_single_config(task_packet):
    model_type, strategy, ode_method, c_att, c_opp, c_clause, gamma, dim_folder = task_packet
    
    pattern = os.path.join(BENCH_DIR, dim_folder, "*.cnf")
    cnf_files = glob.glob(pattern)[:FILES_PER_DIM]
    if not cnf_files:
        return None

    L_num = int(dim_folder.replace("l", ""))
    solved_files_count = 0
    total_satisfied_clauses = 0
    total_clauses_in_dim = 0
    total_time = 0.0

    for filepath in cnf_files:
        cmd = [
            SOLVER_BIN,
            "--file", filepath,
            "--model_type", model_type,
            "--strategy", strategy,
            "--method", ode_method,
            "--c_att", str(c_att),
            "--c_opp", str(c_opp),
            "--c_clause", str(c_clause),
            "--gamma", str(gamma)
        ]
        
        try:
            res = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
            output = res.stdout.strip()
            
            if output and len(output.split(',')) == 4:
                is_solved, sat, total, exec_time = output.split(',')
                solved_files_count += int(is_solved)
                total_satisfied_clauses += int(sat)
                total_clauses_in_dim += int(total)
                total_time += float(exec_time)
        except subprocess.TimeoutExpired:
            total_time += 5.0
            total_clauses_in_dim += (L_num * 4)
            
    if len(cnf_files) == 0 or total_clauses_in_dim == 0:
        return None

    success_rate = (solved_files_count / len(cnf_files)) * 100
    avg_sat_pct = (total_satisfied_clauses / total_clauses_in_dim) * 100
    avg_time = total_time / len(cnf_files)

    return [
        model_type, strategy, ode_method, L_num,
        c_att, c_opp, c_clause, gamma,
        round(success_rate, 2), round(avg_sat_pct, 2), round(avg_time, 4)
    ]

def main():
    print(f"[{datetime.now()}] Старт плотной генерации Grid Search (шаг 0.1)...")
    
    # Считаем общее количество задач математически, чтобы не разворачивать генератор в память
    total_tasks = (len(MODELS) * len(STRATEGIES) * len(ODE_SOLVERS) * len(C_ATT_RANGE) * len(C_OPP_RANGE) * len(C_CLAUSE_RANGE) * len(GAMMA_RANGE) * len(DIMENSIONS))
    
    print(f"[I] Ожидается {total_tasks} комбинаций параметров.")

    # 1. Создаем ИТЕРАТОР (обратите внимание, нет вызова list()!)
    grid_combinations = itertools.product(
        MODELS, STRATEGIES, ODE_SOLVERS, 
        C_ATT_RANGE, C_OPP_RANGE, C_CLAUSE_RANGE, GAMMA_RANGE, 
        DIMENSIONS
    )

    with open(MASTER_CSV, mode='w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            "Model_Type", "Strategy", "Ode_Solver", "L",
            "c_att", "c_opp", "c_clause", "gamma",
            "Success_Rate", "Avg_Satisfied_Pct", "Avg_Time"
        ])

        completed = 0
        
        # 2. Передаем итератор в executor.map с chunksize
        with concurrent.futures.ProcessPoolExecutor() as executor:
            # chunksize=100 заставляет Python выдавать задачи процессору мелкими порциями,
            # ОПЕРАТИВНАЯ ПАМЯТЬ НЕ БУДЕТ ПЕРЕПОЛНЯТЬСЯ
            results = executor.map(evaluate_single_config, grid_combinations, chunksize=100)
            
            for result in results:
                completed += 1
                if result:
                    writer.writerow(result)
                    f.flush() # ПРИНУДИТЕЛЬНО ПИШЕМ СТРОКУ НА ЖЕСТКИЙ ДИСК
                
                if completed % 1000 == 0 or completed == total_tasks:
                    print(f" [{datetime.now()}] Прогресс ультра-сетки: {completed}/{total_tasks} вычислений завершено.")

    print(f"[{datetime.now()}] 🏁 Плотный Grid Search успешно выполнен!")

if __name__ == "__main__":
    main()