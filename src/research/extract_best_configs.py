import pandas as pd

def extract():
    try:
        df = pd.read_csv("grid_search_master.csv")
        
        # Сортируем: сначала по максимуму раскрываемости КНФ, затем по проценту скобок, затем по минимуму времени
        df_sorted = df.sort_values(
            by=["L", "Success_Rate", "Avg_Satisfied_Pct", "Avg_Time"], 
            ascending=[True, False, False, True]
        )
        
        # Группируем по размерности L и берем самую первую (лучшую) конфигурацию
        best_per_dim = df_sorted.groupby("L").first().reset_index()
        
        best_per_dim.to_csv("best_configs.csv", index=False)
        print("Таблица абсолютных лидеров успешно сформирована в 'best_configs.csv':")
        print(best_per_dim.to_string(index=False))
        
    except Exception as e:
        print(f"Ошибка анализа: {e}. Убедитесь, что grid_search_master.csv заполнен.")

if __name__ == "__main__":
    extract()