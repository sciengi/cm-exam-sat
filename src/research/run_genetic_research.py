import os
import glob
import subprocess
import csv
import concurrent.futures
from datetime import datetime

# ================= НАСТРОЙКИ ПУТЕЙ И РЕЖИМОВ =================
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))

RESEARCH_BIN = os.path.join(ROOT_DIR, "build", "research_bench")
BENCH_DIR = os.path.join(ROOT_DIR, "bench", "test")
GENETIC_CSV = os.path.join(ROOT_DIR, "genetic_research_results.csv")

# Конфигурации для исследования
DIMENSIONS = ["l3", "l6", "l9", "l12", "l15", "l18", "l20"]
FILES_PER_DIM = 10  # По 10 жестких файлов на класс

MODEL_TYPE = "3C"            # Альтернатива: "3C"
STRATEGY = "HardStop"        # Альтернатива: "GreedySkip"
ODE_METHOD = "RK4"   # Alternative: "RK4", "RK8"
# =============================================================

def evaluate_single_file_ga(task_packet):
    """Функция-воркер: запускает эволюционный подбор на одном ядре CPU для одного файла"""
    filepath, L_num = task_packet
    
    cmd = [
        RESEARCH_BIN,
        "--file", filepath,
        "--model_type", MODEL_TYPE,
        "--strategy", STRATEGY,
        "--method", ODE_METHOD
    ]
    
    try:
        # Ставим таймаут 120 секунд на эволюцию одной формулы
        res = subprocess.run(cmd, capture_output=True, text=True, check=True, timeout=120)
        output = res.stdout.strip()
        
        # Ищем в консольном выводе бинарника маркер SUCCESS_PARAM
        for line in output.split('\n'):
            if line.startswith("SUCCESS_PARAM,"):
                parts = line.split(',')
                # Извлекаем вещественные гены-победители
                ga_time = float(parts[6])
                c_att = float(parts[7])
                c_opp = float(parts[8])
                c_clause = float(parts[9])
                gamma = float(parts[10])
                
                return [L_num, c_att, c_opp, c_clause, gamma, ga_time]
    except subprocess.TimeoutExpired:
        print(f"  [TIMEOUT] ГА превысил лимит времени для файла: {os.path.basename(filepath)}")
    except Exception as e:
        print(f"  [ERR] Ошибка процесса ГА для файла {os.path.basename(filepath)}: {e}")
    
    return None

def main():
    print(f"[{datetime.now()}] Сбор пула задач для параллельного Генетического Алгоритма...")
    
    # Сначала формируем плоский список пакетов задач
    task_packets = []
    for dim in DIMENSIONS:
        pattern = os.path.join(BENCH_DIR, dim, "*.cnf")
        cnf_files = glob.glob(pattern)[:FILES_PER_DIM]
        L_num = int(dim.replace("l", ""))
        for filepath in cnf_files:
            task_packets.append((filepath, L_num))
            
    total_tasks = len(task_packets)
    print(f"[I] Всего КНФ-задач подготовлено к эволюции: {total_tasks}")
    print(f"[I] Запуск пула процессов. Все ядра процессора активированы...")

    # Инициализируем CSV и пишем шапку
    with open(GENETIC_CSV, mode='w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["L", "c_att", "c_opp", "c_clause", "gamma", "GA_Execution_Time"])

    completed = 0
    # Открываем многопоточный пул процессов на всю мощность вашего CPU
    with concurrent.futures.ProcessPoolExecutor() as executor:
        futures = {executor.submit(evaluate_single_file_ga, packet): packet for packet in task_packets}
        
        for future in concurrent.futures.as_completed(futures):
            completed += 1
            result = future.result()
            
            if result:
                # Асинхронно и потокобезопасно записываем строку в CSV на лету
                with open(GENETIC_CSV, mode='a', newline='') as f_append:
                    csv.writer(f_append).writerow(result)
            
            if completed % 5 == 0 or completed == total_tasks:
                print(f"  [{datetime.now()}] Эволюционный прогресс выборки: {completed}/{total_tasks} файлов обработано.")

    print(f"\n[{datetime.now()}] 🏁 Параллельное генетическое исследование успешно завершено!")
    print(f"[I] Результаты логов сохранены в: '{GENETIC_CSV}'")

if __name__ == "__main__":
    main()