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

    # Преобразуем числовые типы данных
    df['Is_Solved'] = pd.to_numeric(df['Is_Solved'], errors='coerce').fillna(0).astype(int)
    df['Variables_L'] = df['Variables_L'].astype(int)
    df['Laws_Count'] = df['Laws_Count'].astype(int)
    df['Time_Seconds'] = pd.to_numeric(df['Time_Seconds'], errors='coerce').fillna(0)

    L_values = sorted(df['Variables_L'].unique())
    methods_to_plot = ["Leapfrog", "RK4", "DP8Adaptive"]

    # Создаем большой холст: строки — это методы ОДУ, столбцы — Метрика 1 (Раскрываемость) и Метрика 2 (Время)
    fig, axes = plt.subplots(len(methods_to_plot), 2, figsize=(14, 4 * len(methods_to_plot)), sharex=True)
    fig.suptitle('Итоговые результаты Ablation Study: Влияние физических законов и интеграторов ОДУ', fontsize=16, y=0.98)

    laws_style = {
        1: {'label': '1 Закон (Только гравитация)', 'color': '#e74c3c', 'marker': 'o'},
        2: {'label': '2 Закона (+ Трение)', 'color': '#e67e22', 'marker': 's'},
        3: {'label': '3 Закона (+ Расталкивание дизъюнктов)', 'color': '#2ecc71', 'marker': '^'}
    }

    for idx, method in enumerate(methods_to_plot):
        df_method = df[df['OdeMethod'] == method]
        ax_success = axes[idx, 0]
        ax_time = axes[idx, 1]

        ax_success.set_title(f"Интегратор: {method} | Раскрываемость", fontsize=11, weight='bold')
        ax_time.set_title(f"Интегратор: {method} | Асимптотика времени", fontsize=11, weight='bold')

        for laws, style in laws_style.items():
            df_laws = df_method[df_method['Laws_Count'] == laws]
            if df_laws.empty:
                continue

            grouped = df_laws.groupby('Variables_L')
            
            # Расчет процента успешных SAT решений (Метрика 1)
            success_rate = grouped['Is_Solved'].mean() * 100
            ax_success.plot(success_rate.index, success_rate.values, 
                             label=style['label'], color=style['color'], 
                             marker=style['marker'], linewidth=2, markersize=6)

            # Расчет среднего времени решения (Метрика 2)
            mean_time = grouped['Time_Seconds'].mean()
            std_time = grouped['Time_Seconds'].std().fillna(0)
            ax_time.errorbar(mean_time.index, mean_time.values, yerr=std_time.values,
                             label=style['label'], color=style['color'], 
                             marker=style['marker'], fmt='-', linewidth=1.5, capsize=3, markersize=6)

        # Оформление панелей
        ax_success.set_ylabel('Успешность реш. (%)', fontsize=10)
        ax_success.set_ylim(-5, 105)
        ax_success.grid(True, linestyle='--', alpha=0.5)
        
        ax_time.set_ylabel('Время (секунды)', fontsize=10)
        ax_time.grid(True, linestyle='--', alpha=0.5)

    # Оформление нижней оси для последних панелей
    axes[-1, 0].set_xlabel('Количество булевых переменных (L)', fontsize=12)
    axes[-1, 1].set_xlabel('Количество булевых переменных (L)', fontsize=12)
    
    # Расставляем тики по оси X
    for ax in axes.flat:
        ax.set_xticks(L_values)

    # Добавляем единую легенду внизу графиков
    handles, labels = axes[0, 0].get_legend_handles_labels()
    fig.legend(handles, labels, loc='lower center', ncol=3, fontsize=12, bbox_to_anchor=(0.5, 0.01))

    plt.tight_layout(rect=[0, 0.05, 1, 0.95])
    plt.savefig('final_ablation_study_report.png', dpi=300)
    print("Комплексный валидационный отчет сохранен в 'final_ablation_study_report.png'")
    plt.show()

if __name__ == "__main__":
    main()