import os
import glob
from pysat.solvers import Minisat22
from pysat.formula import CNF

def parse_dimacs(filepath):
    """Собственный безопасный парсер для обхода символа % в старых файлах SATLIB"""
    clauses = []
    current_clause = []
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            # Пропускаем комментарии и заголовок
            if line.startswith('c') or line.startswith('p'):
                continue
            # Если встретили % - это конец старого формата DIMACS, прекращаем чтение
            if line == '%':
                break
            
            # Читаем числа
            for token in line.split():
                try:
                    val = int(token)
                    if val == 0:
                        clauses.append(current_clause)
                        current_clause = []
                    else:
                        current_clause.append(val)
                except ValueError:
                    pass # Игнорируем любой другой мусор, если он есть
    return clauses

# Ищем все .cnf файлы
cnf_files = glob.glob("bench/test/*.cnf")
output_file = "bench/test/ground_truth.txt"

print(f"Найдено {len(cnf_files)} файлов. Начинаю вычисления...")

with open(output_file, "w") as out:
    for filepath in cnf_files:
        filename = os.path.basename(filepath)
        try:
            # 1. Читаем нашим безопасным парсером
            clauses = parse_dimacs(filepath)
            # 2. Передаем готовые массивы в библиотеку
            formula = CNF(from_clauses=clauses)
            
            # 3. Решаем
            with Minisat22(bootstrap_with=formula) as solver:
                if solver.solve():
                    model = solver.get_model()
                    binary_model = "".join(["1" if val > 0 else "0" for val in model])
                    out.write(f"{filename} {binary_model}\n")
                    print(f"Решено: {filename}")
                else:
                    print(f"UNSAT: {filename}")
        except Exception as e:
            print(f"Ошибка при обработке {filename}: {e}")

print(f"✅ Все эталонные ответы успешно сохранены в {output_file}")