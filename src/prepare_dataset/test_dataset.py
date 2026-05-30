import os
from pysat.solvers import Minisat22
from pysat.formula import CNF as SatCNF

# Настройки путей относительно src/prepare_dataset/
ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
BENCH_DIR = os.path.join(ROOT_DIR, "bench", "test")
GROUND_TRUTH_FILE = os.path.join(ROOT_DIR, "ground_truth.txt")

EXPECTED_CLASSES = [3, 6, 9, 12, 15, 18]
EXPECTED_FILES_PER_CLASS = 100

def parse_dimacs_clauses(filepath):
    """Парсит cnf файл и возвращает список дизъюнктов"""
    clauses = []
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('c') or line.startswith('p'):
                continue
            # Считываем литералы до нуля
            literals = [int(x) for x in line.split() if x != '0']
            if literals:
                clauses.append(literals)
    return clauses

def verify_bitmask_solution(clauses, bitmask):
    """Проверяет, удовлетворяет ли битовая маска из кэша список дизъюнктов"""
    # Преобразуем строку '10110' в булев список, где индекс 0 соответствует переменной 1
    model = [True if char == '1' else False for char in bitmask]
    
    for clause in clauses:
        clause_satisfied = False
        for lit in clause:
            var_idx = abs(lit) - 1
            # Защита от выхода за границы
            if var_idx >= len(model):
                return False
            val = model[var_idx]
            
            if (lit > 0 and val) or (lit < 0 and not val):
                clause_satisfied = True
                break
        if not clause_satisfied:
            return False # Если хотя бы одна скобка оказалась False, решение неверно
    return True

def run_tests():

    if not os.path.exists(GROUND_TRUTH_FILE):
        print(f"Глобальный файл ответов {GROUND_TRUTH_FILE} не найден!")
        return
        
    # Считываем кэш ответов в словарь для быстрой проверки
    gt_cache = {}
    with open(GROUND_TRUTH_FILE, 'r') as f:
        for line in f:
            if line.strip():
                rel_path, mask = line.strip().split()
                gt_cache[rel_path] = mask

    total_errors = 0
    checked_files_count = 0

    for L in EXPECTED_CLASSES:
        class_folder = os.path.join(BENCH_DIR, f"l{L}")
        if not os.path.exists(class_folder):
            print(f"Подпапка класса 'l{L}' отсутствует на диске!")
            total_errors += 1
            continue
            
        cnf_files = sorted([f for f in os.listdir(class_folder) if f.endswith('.cnf')])
        
        if len(cnf_files) != EXPECTED_FILES_PER_CLASS:
            print(f"Класс l{L}: Найдено {len(cnf_files)} файлов вместо {EXPECTED_FILES_PER_CLASS}!")
            total_errors += 1
            
        # Запускаем внутреннюю математическую проверку разрешимости для каждого файла
        class_math_errors = 0
        for filename in cnf_files:
            rel_path_key = f"l{L}/{filename}"
            filepath = os.path.join(class_folder, filename)
            
            # 1. Читаем структуру дизъюнктов
            clauses = parse_dimacs_clauses(filepath)
            
            # 2. Проверяем реальную разрешимость файла через PySAT
            formula = SatCNF(from_clauses=clauses)
            with Minisat22(bootstrap_with=formula) as solver:
                is_actually_sat = solver.solve()
                
            if not is_actually_sat:
                print(f"  [ERR] Файл {rel_path_key} математически UNSAT (не имеет решений)!")
                class_math_errors += 1
                continue
                
            # 3. Проверяем валидность строки решения в ground_truth.txt
            if rel_path_key not in gt_cache:
                print(f"  [ERR] Файл {rel_path_key} отсутствует в общем кэше ground_truth.txt!")
                class_math_errors += 1
                continue
                
            saved_mask = gt_cache[rel_path_key]
            if not verify_bitmask_solution(clauses, saved_mask):
                print(f"  [ERR] Битовая маска [{saved_mask}] для файла {rel_path_key} ложна и не решает КНФ!")
                class_math_errors += 1
                continue
                
            checked_files_count += 1
            
        if class_math_errors == 0:
            print(f"Класс l{L}: Все {EXPECTED_FILES_PER_CLASS} файлов проверены. 100% SAT, кэш верифицирован.")
        else:
            print(f"Класс l{L}: Обнаружено {class_math_errors} математических ошибок рендеринга формул.")
            total_errors += class_math_errors

    if total_errors == 0:
        print(f"\nИТОГ: Успешно пропалировано {checked_files_count} SAT-задач.")
        print(" Все формулы строго выполнимы, а эталонные маски верны. База данных не содержит ошибок!")
    else:
        print(f"ИТОГ: Тестирование провалено. Обнаружено ошибок: {total_errors}")

if __name__ == "__main__":
    run_tests()