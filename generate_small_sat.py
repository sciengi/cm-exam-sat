import os
import random

def generate_random_3sat(num_vars, num_clauses, filepath):
    """
    Генерирует случайную 3-КНФ формулу.
    num_vars  — число переменных (L)
    num_clauses — требуемое число дизъюнктов
    filepath  — путь для сохранения файла
    """
    clauses = []
    while len(clauses) < num_clauses:
        # 3 переменные
        vars_sample = random.sample(range(1, num_vars + 1), 3)
        # Случайное отрицание
        clause = [v if random.random() > 0.5 else -v for v in vars_sample]
        # Проверяем уникальность (без учёта порядка литералов)
        if sorted(clause) not in [sorted(c) for c in clauses]:
            clauses.append(clause)

    with open(filepath, 'w') as f:
        f.write("c Generated 3-SAT task\n")
        f.write(f"p cnf {num_vars} {num_clauses}\n")
        for c in clauses:
            f.write(f"{c[0]} {c[1]} {c[2]} 0\n")


# Создаём папку, если её нет
os.makedirs("bench/test", exist_ok=True)

# Точные характеристики по заданию: L -> число дизъюнктов
task_specs = { 
    3: 6,
    5: 21,
    7: 30,
    9: 38,
    12: 51,
    15: 64
}

NUM_FILES = 20   # по 20 файлов на каждый размер

for L, C in task_specs.items():
    print(f"Генерация uf{L}: {L} переменных, {C} дизъюнктов ({NUM_FILES} файлов)...")
    for i in range(NUM_FILES):
        filename = f"bench/test/uf{L}-{i:02d}.cnf"
        generate_random_3sat(L, C, filename)

print("Все файлы успешно созданы в bench/test/")