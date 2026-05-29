import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

CSV_FILE = "final_validation_results.csv"

def main():
    try:
        df = pd.read_csv(CSV_FILE)
    except FileNotFoundError:
        print(f"[ERR] Файл {CSV_FILE} не найден. Сначала запустите run_final_validation.py")
        return

    # Конвертируем типы данных в числовые
    df['Variables_L'] = df['Variables_L'].astype(int)
    df['Laws_Count'] = df['Laws_Count'].astype(int)
    df['Sat_Clauses'] = pd.to_numeric(df['Sat_Clauses'], errors='coerce').fillna(0)
    df['Total_Clauses'] = pd.to_numeric(df['Total_Clauses'], errors='coerce').fillna(1)

    # Вычисляем новую метрику: процент успешно удовлетворенных дизъюнктов
    df['Sat_Percentage'] = (df['Sat_Clauses'] / df['Total_Clauses']) * 100

    L_values = sorted(df['Variables_L'].unique())
    methods_to_plot = ["Leapfrog", "RK4", "DP8Adaptive"]

    fig, axes = plt.subplots(1, len(methods_to_plot), figsize=(18, 5), sharey=True)
    fig.suptitle('Метрика качества сходимости ОДУ: Средний процент выполненных дизъюнктов КНФ', fontsize=15, y=0.98)

    laws_style = {
        1: {'label': '1 Закон (Чистая гравитация)', 'color': '#e74c3c', 'marker': 'o'},
        2: {'label': '2 Закона (+ Трение)', 'color': '#e67e22', 'marker': 's'},
        3: {'label': '3 Закона (+ Расталкивание дизъюнктов)', 'color': '#2ecc71', 'marker': '^'}
    }

    for idx, method in enumerate(methods_to_plot):
        ax = axes[idx]
        df_method = df[df['OdeMethod'] == method]
        ax.set_title(f"Интегратор: {method}", fontsize=12, weight='bold')

        for laws, style in laws_style.items():
            df_laws = df_method[df_method['Laws_Count'] == laws]
            if df_laws.empty:
                continue

            # Группируем по размерности L и находим СРЕДНИЙ ПРОЦЕНТ скобок
            grouped = df_laws.groupby('Variables_L')
            mean_sat_pct = grouped['Sat_Percentage'].mean()
            
            ax.plot(mean_sat_pct.index, mean_sat_pct.values, 
                    label=style['label'], color=style['color'], 
                    marker=style['marker'], linewidth=2.5, markersize=6)

        ax.set_xlabel('Количество переменных (L)', fontsize=11)
        if idx == 0:
            ax.set_ylabel('Выполненные дизъюнкты (%)', fontsize=11)
        ax.set_ylim(40, 105) # Нижний порог 40%, так как случайное решение 3-SAT дает ~87.5%
        ax.grid(True, linestyle='--', alpha=0.6)

    # Ставим легенду в самом низу
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc='lower center', ncol=3, fontsize=11, bbox_to_anchor=(0.5, -0.02))

    plt.tight_layout(rect=[0, 0.08, 1, 0.92])
    plt.savefig('satisfied_clauses_percentage.png', dpi=300)
    print("📈 Новый график успешно сохранен в 'satisfied_clauses_percentage.png'")
    plt.show()

if __name__ == "__main__":
    main()