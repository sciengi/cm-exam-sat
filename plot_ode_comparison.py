import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

CSV_FILE = "optimized_physics_parameters.csv"

def main():
    try:
        df = pd.read_csv(CSV_FILE)
    except FileNotFoundError:
        print(f"[ERR] Файл {CSV_FILE} не найден.")
        return

    methods_style = {
        "Leapfrog": {"color": "blue", "marker": "s", "label": "Leapfrog (Симплектический)"},
        "RK4": {"color": "green", "marker": "o", "label": "RK4 (Классический)"},
        "DP8": {"color": "red", "marker": "x", "label": "DP8 (Фиксированный шаг)"},
        "DP8Adaptive": {"color": "purple", "marker": "^", "label": "DP8Adaptive (Адаптивный)"}
    }

    plt.figure(figsize=(10, 6))
    L_values = sorted(df['L'].unique())

    for method, style in methods_style.items():
        subset = df[df['OdeMethod'] == method]
        if subset.empty:
            continue

        grouped = subset.groupby('L')
        mean_time = grouped['GA_Time_Seconds'].mean()
        std_time = grouped['GA_Time_Seconds'].std().fillna(0)

        # Вывод графика с усами стандартного отклонения
        plt.errorbar(mean_time.index, mean_time.values, yerr=std_time.values,
                     label=style["label"], color=style["color"], marker=style["marker"],
                     fmt='-', linewidth=2, capsize=4, markersize=7)

    plt.title('Сравнительный анализ численных интеграторов ОДУ в ГА-оптимизации', fontsize=14)
    plt.xlabel('Количество переменных КНФ (L)', fontsize=12)
    plt.ylabel('Среднее время оптимизации параметров (секунды)', fontsize=12)
    plt.xticks(L_values)
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend(fontsize=10)
    
    plt.tight_layout()
    plt.savefig('ode_method_comparison.png', dpi=300)
    print("📈 График успешно сохранен в 'ode_method_comparison.png'")
    plt.show()

if __name__ == "__main__":
    main()