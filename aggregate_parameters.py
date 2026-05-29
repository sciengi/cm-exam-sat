import pandas as pd

CSV_FILE = "optimized_physics_parameters.csv"

def main():
    try:
        df = pd.read_csv(CSV_FILE)
    except FileNotFoundError:
        print(f"[ERR] Файл {CSV_FILE} не найден. Проверьте путь.")
        return

    print("=========================================================================")
    print(" АГРЕГИРОВАННЫЕ ОПТИМАЛЬНЫЕ ГИПЕРПАРАМЕТРЫ ДЛЯ КЛАССОВ КНФ (СРЕДНЕЕ) ")
    print("=========================================================================")
    
    df_clean = df[df['GA_Time_Seconds'] > 0]
    
    aggregated = df_clean.groupby('L')[['c_att', 'c_opp', 'c_clause', 'gamma']].mean()
    
    print(aggregated.round(3).to_string())
    print("=========================================================================")
    
    aggregated.round(3).to_csv("final_class_parameters.csv")
    print("[I] Сводная таблица параметров сохранена в 'final_class_parameters.csv'")

if __name__ == "__main__":
    main()
