import os
import glob
import subprocess
import concurrent.futures
import csv
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime

# Настройки
SOLVER_BIN = "./build/canonical_solver"
BENCH_DIR = "bench/test/"
OUTPUT_CSV = "canonical_results.csv"
DIMENSIONS = ["uf3", "uf5", "uf7", "uf9", "uf12"]
FILES_PER_DIM = 10
METHODS = ["RK4", "Leapfrog", "DP8Adaptive"]

def run_task(args):
    filepath, method = args
    cmd = [SOLVER_BIN, "--file", filepath, "--method", method]
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=True, timeout=15)
        return res.stdout.strip()
    except:
        # Аварийный случай: Is_Solved=0, Sat=0, Total=100
        l_num = os.path.basename(filepath).split('-')[0].replace('uf', '')
        return f"0,{filepath},{l_num},0,100,15.0"

def main():
    print(f"[{datetime.now()}] Сбор канонических задач...")
    tasks = []
    for dim in DIMENSIONS:
        pattern = os.path.join(BENCH_DIR, f"{dim}-*.cnf")
        files = glob.glob(pattern)[:FILES_PER_DIM]
        for f in files:
            for m in METHODS:
                tasks.append((f, m))

    print(f"Всего задач для канонического ОДУ: {len(tasks)}")
    
    with open(OUTPUT_CSV, mode='w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["Is_Solved", "Filepath", "L", "Sat_Clauses", "Total_Clauses", "Time", "Method"])

    with concurrent.futures.ProcessPoolExecutor() as executor:
        futures = {executor.submit(run_task, t): t for t in tasks}
        for future in concurrent.futures.as_completed(futures):
            task = futures[future]
            line = future.result()
            if line:
                parts = line.split(',')
                # Дописываем имя метода в конец строки
                with open(OUTPUT_CSV, mode='a', newline='') as f:
                    csv.writer(f).writerow(parts + [task[1]])

    print(f"[{datetime.now()}] Сбор данных завершен. Строим графики...")
    
    df = pd.read_csv(OUTPUT_CSV)
    df['Sat_Percentage'] = (df['Sat_Clauses'] / df['Total_Clauses']) * 100
    
    plt.figure(figsize=(9, 5))
    for m in METHODS:
        sub = df[df['Method'] == m]
        grouped = sub.groupby('L')['Sat_Percentage'].mean()
        plt.plot(grouped.index, grouped.values, marker='o', linewidth=2, label=f"Канонический {m} (3C тел)")
        
    plt.axhline(y=100, color='r', linestyle='--', alpha=0.7, label='Идеальный SAT (100%)')
    plt.title('Эффективность канонического метода Матиясевича ($3C$ тел)', fontsize=13)
    plt.xlabel('Количество булевых переменных (L)', fontsize=11)
    plt.ylabel('Средний процент выполненных дизъюнктов (%)', fontsize=11)
    plt.ylim(50, 105)
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.legend(loc='lower left')
    plt.tight_layout()
    plt.savefig('canonical_method_efficiency.png', dpi=300)
    print("График успешно сохранен в 'canonical_method_efficiency.png'")
    plt.show()

if __name__ == "__main__":
    main()