import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Имя файла с результатами
CSV_FILE = "experiment_results.csv"

def main():
    try:
        # Читаем данные
        df = pd.read_csv(CSV_FILE)
    except FileNotFoundError:
        print(f"[ERR] Файл {CSV_FILE} не найден. Сначала запустите ЭТАП 2.")
        return

    # Названия для легенды и цвета линий
    laws_info = {
        1: {'label': '1 закон (Гравитация)', 'color': 'red', 'marker': 'o'},
        2: {'label': '2 закона (+ Трение)', 'color': 'orange', 'marker': 's'},
        3: {'label': '3 закона (+ Расталкивание дизъюнктов)', 'color': 'green', 'marker': '^'}
    }

    # Создаем холст с двумя графиками (один под другим)
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 12), sharex=True)
    fig.suptitle('Анализ пределов применимости физического метода (Ablation Study)', fontsize=16)

    # Уникальные размерности (L), отсортированные по возрастанию
    L_values = sorted(df['Variables_L'].unique())

    for laws_count, info in laws_info.items():
        # Отбираем данные только для текущего количества законов
        subset = df[df['Laws_Count'] == laws_count]
        if subset.empty:
            continue

        # Группируем по числу переменных L
        grouped = subset.groupby('Variables_L')

        # -------------------------------------------------------------
        # ГРАФИК 1: Раскрываемость (Процент успешно решенных задач)
        # -------------------------------------------------------------
        # Is_Solved содержит 1 (решено) или 0 (не решено). Среднее * 100 даст процент.
        success_rate = grouped['Is_Solved'].mean() * 100
        
        ax1.plot(success_rate.index, success_rate.values, 
                 label=info['label'], color=info['color'], 
                 marker=info['marker'], linewidth=2, markersize=8)

        # -------------------------------------------------------------
        # ГРАФИК 2: Время решения (со стандартным отклонением)
        # -------------------------------------------------------------
        # Считаем среднее время и дисперсию (std)
        mean_time = grouped['Time_Seconds'].mean()
        std_time = grouped['Time_Seconds'].std()

        # Заполняем NaN нулями (если для какого-то L была всего 1 точка)
        std_time = std_time.fillna(0)

        # Строим график с "усами" ошибок (errorbars)
        ax2.errorbar(mean_time.index, mean_time.values, yerr=std_time.values, 
                     label=info['label'], color=info['color'], 
                     marker=info['marker'], fmt='-', linewidth=2, capsize=5, markersize=8)

    # --- Оформление Графика 1 ---
    ax1.set_ylabel('Успешно решенные задачи (%)', fontsize=12)
    ax1.set_title('Метрика 1: Способность метода найти решение (Раскрываемость)', fontsize=12)
    ax1.set_ylim(-5, 105)
    ax1.grid(True, linestyle='--', alpha=0.7)
    ax1.legend(fontsize=10)

    # --- Оформление Графика 2 ---
    ax2.set_xlabel('Количество переменных (L)', fontsize=12)
    ax2.set_ylabel('Среднее время решения (секунды)', fontsize=12)
    ax2.set_title('Метрика 2: Затраченное время с разбросом (Асимптотика сложности)', fontsize=12)
    ax2.set_xticks(L_values)
    ax2.grid(True, linestyle='--', alpha=0.7)
    ax2.legend(fontsize=10)

    ax1.set_xscale('log')
    ax2.set_xscale('log')

    ax2.set_xticks(L_values)
    ax2.set_xticklabels([str(x) for x in L_values])
    # Сохраняем картинку в высоком разрешении и показываем на экране
    plt.tight_layout()
    plt.savefig('exam_results_plot.png', dpi=300)
    print("[I] Графики успешно построены и сохранены в 'exam_results_plot.png'")
    plt.show()

if __name__ == "__main__":
    main()