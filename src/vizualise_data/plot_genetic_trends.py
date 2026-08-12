import os
import pandas as pd
import matplotlib.pyplot as plt
from scipy.stats import linregress

# Автоматическое определение абсолютных путей от корня репозитория
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
GENETIC_CSV = os.path.join(ROOT_DIR, "genetic_research_results_2L_RK4_Greedy.csv")
TREND_PNG = os.path.join(ROOT_DIR, "genetic_parameter_trends.png")

def main():
    if not os.path.exists(GENETIC_CSV):
        print(f"Файл {GENETIC_CSV} не найден! Запустите сначала run_genetic_research.py")
        return

    # Загружаем собранный эволюционный датасет
    df = pd.read_csv(GENETIC_CSV)
    if df.empty:
        print("Файл результатов пуст.")
        return

    print(f"[I] Успешно загружено {len(df)} строк эволюционных вычислений.")

    # Вычисляем среднее значение параметров для каждого класса размерности L
    grouped = df.groupby("L").mean().reset_index()

    # Инициализируем графическое окно
    plt.figure(figsize=(11, 6.5))
    
    # Цветовая палитра и подписи для отчета
    colors = {'c_att': '#1f77b4', 'c_opp': '#d62728', 'c_clause': '#2ca02c', 'gamma': '#9467bd'}
    labels = {
        'c_att': 'Притяжение клонов ($c_{att}$)',
        'c_opp': 'Отталкивание антиподов ($c_{opp}$)',
        'c_clause': 'Расталкивание скобок ($c_{clause}$)',
        'gamma': 'Трение среды ($\\gamma$)'
    }

    # Итерируемся по физическим параметрам сил
    for param in ['c_att', 'c_opp', 'c_clause', 'gamma']:
        # Отображаем маркеры реальных средних значений
        plt.scatter(grouped["L"], grouped[param], color=colors[param], s=60, zorder=3, edgecolor='black')
        
        # Математический расчет линейной регрессии тренда
        slope, intercept, r_value, p_value, std_err = linregress(grouped["L"], grouped[param])
        line_x = grouped["L"]
        line_y = slope * line_x + intercept
        
        # Отрисовка аппроксимирующей линии
        plt.plot(
            line_x, line_y, 
            color=colors[param], linestyle='--', alpha=0.85, linewidth=1.8,
            label=f"{labels[param]} [R²={r_value**2:.2f}]"
        )

    # Геометрическое оформление осей и заголовков для вычмат-презентации
    plt.title("Эволюционные тренды изменения физических сил от сложности задачи $L$\n(По результатам чистой фитнес-функции без Хэмминга)", fontsize=11, pad=15)
    plt.xlabel("Количество булевых переменных КНФ (Размерность задачи L)", fontsize=10)
    plt.ylabel("Оптимальное значение вещественного коэффициента", fontsize=10)
    
    plt.xticks(grouped["L"]) # Строго фиксируем засечки по нашим L [3, 6, 9, 12, 15, 18]
    plt.grid(True, linestyle=':', alpha=0.6, zorder=1)
    plt.legend(loc="upper left", fontsize=9, framealpha=0.9)
    
    # Сохраняем в высоком качестве
    plt.savefig(TREND_PNG, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Научный аналитический график успешно сгенерирован и сохранен в: '{TREND_PNG}'")

if __name__ == "__main__":
    main()