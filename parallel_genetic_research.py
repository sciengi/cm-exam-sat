import os
import glob
import subprocess
import concurrent.futures
import csv
from datetime import datetime

# ================= НАСТРОЙКИ СЕРВЕРА =================
RESEARCH_BIN = "./build/research_bench" 
CACHE_TXT = "ground_truth.txt"
BENCH_DIR = "bench/test/"
OUTPUT_PARAMS_CSV = "optimized_physics_parameters.csv"

# Диапазон под микро-КНФ
DIMENSIONS = ["uf3", "uf5", "uf7", "uf9", "uf10", "uf12", "uf15", "uf20"]
FILES_PER_DIM = 20

# Тестируемые методы численной интеграции
METHODS_TO_TEST = ["RK4", "DP8", "Leapfrog", "DP8Adaptive"]

# ЖЕСТКИЙ ТАЙМАУТ на один запуск ГА (в секундах)
# Если C++ зависнет из-за сингулярности, Python убьет его через 20 сек.
TASK_TIMEOUT = 180.0 
# =====================================================

def run_ga_for_file(task_args):
    filepath, method = task_args
    filename = os.path.basename(filepath)
    
    # Сразу пишем в консоль, что задача пошла в работу
    print(f"[{datetime.now().strftime('%H:%M:%S')}] [START] {filename} | Метод: {method}")
    
    cmd = [RESEARCH_BIN, "--file", filepath, "--cache", CACHE_TXT, "--method", method]
    start_time = datetime.now()
    
    try:
        # Запускаем с ограничением времени выполнения timeout
        result = subprocess.run(cmd, capture_output=True, text=True, check=True, timeout=TASK_TIMEOUT)
        elapsed = (datetime.now() - start_time).total_seconds()
        
        for line in result.stdout.split('\n'):
            if line.startswith("SUCCESS_PARAM,"):
                print(f"[{datetime.now().strftime('%H:%M:%S')}] [ OK  ] {filename} | {method} завершен за {elapsed:.2f}с")
                return line.strip()
                
    except subprocess.TimeoutExpired:
        print(f"[{datetime.now().strftime('%H:%M:%S')}] [TIME!] {filename} | {method} ПРЕВЫСИЛ ТАЙМАУТ {TASK_TIMEOUT}с (Процесс убит)")
    except subprocess.CalledProcessError as e:
        print(f"[{datetime.now().strftime('%H:%M:%S')}] [FAIL ] {filename} | {method} завершился с ошибкой ядра C++")
    except Exception as e:
        print(f"[{datetime.now().strftime('%H:%M:%S')}] [ERR  ] {filename} | Неизвестная ошибка: {e}")
        
    return f"TIMEOUT_OR_ERR,{filepath},0,{method},0,0,0,0,0"

def main():
    print(f"[{datetime.now()}] Сканирование папок КНФ...")
    tasks = []
    for dim in DIMENSIONS:
        # Ищем строго файлы нужной размерности, добавляя дефис (например, "uf5-")
        pattern = os.path.join(BENCH_DIR, f"{dim}-*.cnf")
        files = glob.glob(pattern)[:FILES_PER_DIM]
        for f in files:
            for method in METHODS_TO_TEST:
                tasks.append((f, method))
        
    total_tasks = len(tasks)
    print(f"[{datetime.now()}] Сформировано задач для обсчета: {total_tasks}")
    
    if not os.path.exists(CACHE_TXT):
        print(f"[ERR] Файл {CACHE_TXT} не найден в корне проекта. Отмена.")
        return

    # Открываем CSV и пишем шапку
    with open(OUTPUT_PARAMS_CSV, mode='w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["Filepath", "L", "OdeMethod", "GA_Time_Seconds", "c_att", "c_opp", "c_clause", "gamma"])

    print(f"[{datetime.now()}] Разворачиваем многопроцессорный пул на сервере...\n")
    completed = 0
    
    with concurrent.futures.ProcessPoolExecutor() as executor:
        future_to_task = {executor.submit(run_ga_for_file, t): t for t in tasks}
        
        for future in concurrent.futures.as_completed(future_to_task):
            task = future_to_task[future]
            res_line = future.result()
            
            if res_line and not res_line.startswith("TIMEOUT_OR_ERR"):
                parts = res_line.split(',')[1:]
                with open(OUTPUT_PARAMS_CSV, mode='a', newline='') as f:
                    writer = csv.writer(f)
                    writer.writerow(parts)
            
            completed += 1
            if completed % 5 == 0 or completed == total_tasks:
                print(f"\n--- СИСТЕМНЫЙ ПРОГРЕСС: {completed}/{total_tasks} задач обработано ---\n")

    print(f"[{datetime.now()}] Эксперимент успешно завершен! Результаты: '{OUTPUT_PARAMS_CSV}'")

if __name__ == "__main__":
    main()