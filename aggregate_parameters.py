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
    
    # Группируем данные по числу переменных L и считаем среднее для физических сил
    # Фильтруем успешные методы (исключаем возможные строки ошибок, если они записались)
    df_clean = df[df['GA_Time_Seconds'] > 0]
    
    aggregated = df_clean.groupby('L')[['c_att', 'c_opp', 'c_clause', 'gamma']].mean()
    
    # Округляем до 3 знаков для красивого академического вида
    print(aggregated.round(3).to_string())
    print("=========================================================================")
    
    # Сохраняем агрегированную таблицу в текстовый файл для отчета
    aggregated.round(3).to_csv("final_class_parameters.csv")
    print("[I] Сводная таблица параметров сохранена в 'final_class_parameters.csv'")

if __name__ == "__main__":
    main()