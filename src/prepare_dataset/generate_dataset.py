import os
import random
import shutil
from pysat.solvers import Minisat22
from pysat.formula import CNF as SatCNF

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
BENCH_DIR = os.path.join(ROOT_DIR, "bench", "test")
GROUND_TRUTH_FILE = os.path.join(ROOT_DIR, "ground_truth.txt")

# Исследуемый компактный ряд классов КНФ
LAWS_VARIABLES = [3, 6, 9, 12, 15, 18]
FILES_PER_DIM = 100  # По 100 штук на каждый класс

# Асимптотический коэффициент максимальной сложности (для L >= 6)
ALPHA = 4.3

def clear_old_garbage():
    """Полностью стирает старые файлы бенчмарков и кэш эталонов для порядка"""
    print("[I] Очистка репозитория от старых файлов...")
    if os.path.exists(BENCH_DIR):
        shutil.rmtree(BENCH_DIR)
    os.makedirs(BENCH_DIR, exist_ok=True)
    if os.path.exists(GROUND_TRUTH_FILE):
        try: os.remove(GROUND_TRUTH_FILE)
        except: pass

def generate_random_3sat_clause(num_vars):
    """Генератор одной случайной 3-литеральной скобки"""
    vars_sample = random.sample(range(1, num_vars + 1), 3)
    return [v if random.random() > 0.5 else -v for v in vars_sample]

def main():
    clear_old_garbage()
    print(f"[START] Изолированная генерация (по {FILES_PER_DIM} файлов максимальной сложности на класс)...")
    
    with open(GROUND_TRUTH_FILE, mode='w') as gt_file:
        for L in LAWS_VARIABLES:
            
            # Считаем число дизъюнктов с учетом дискретного эффекта малых размерностей
            if L == 3:
                C = 6   # Канонический порог сложности SATLIB для L=3
            else:
                C = int(round(ALPHA * L)) # Честный жесткий порог 4.3 для остальных
                
            # Создаем изолированную подпапку для класса
            class_dir = os.path.join(BENCH_DIR, f"l{L}")
            os.makedirs(class_dir, exist_ok=True)
            
            print(f"\n[CLASS L={L}] Фиксировано дизъюнктов C={C} (Эфф. коэффициент = {round(C/L, 2)}) -> Папка: bench/test/l{L}/")
            
            generated_success = 0
            while generated_success < FILES_PER_DIM:
                clauses = []
                while len(clauses) < C:
                    clause = generate_random_3sat_clause(L)
                    if sorted(clause) not in [sorted(c) for c in clauses]:
                        clauses.append(clause)
                
                # Фильтрация невыполнимых формул на лету через PySAT
                formula = SatCNF(from_clauses=clauses)
                with Minisat22(bootstrap_with=formula) as solver:
                    if solver.solve():
                        generated_success += 1
                        filename = f"uf{L}-{generated_success}.cnf"
                        filepath = os.path.join(class_dir, filename)
                        
                        # Сохраняем КНФ в персональную папку класса
                        with open(filepath, 'w') as f:
                            f.write(f"c Hard Clean Generated 3-SAT\n")
                            f.write(f"p cnf {L} {C}\n")
                            for c in clauses:
                                f.write(f"{c[0]} {c[1]} {c[2]} 0\n")
                        
                        # Вытаскиваем одно гарантированное булево решение
                        model = solver.get_model()
                        binary_model = "".join(["1" if val > 0 else "0" for val in model])
                        
                        # Записываем относительный путь с учетом папки класса в ground_truth.txt
                        relative_path_entry = f"l{L}/{filename}"
                        gt_file.write(f"{relative_path_entry} {binary_model}\n")
                        
                        if generated_success % 25 == 0:
                            print(f"  -> Сгенерировано SAT-файлов: {generated_success}/{FILES_PER_DIM}")
                            
    print(f"\n[SUCCESS] Новый изолированный датасет успешно построен с опорой на стандарты SATLIB!")

if __name__ == "__main__":
    main()