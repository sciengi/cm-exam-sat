import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
ABLATION_CSV = os.path.join(ROOT_DIR, "ablation_results.csv")

# Конфигурация стилей для научного отчета
COLORS = {1: '#e31a1c', 2: '#ff7f00', 3: '#33a02c'}  # Красный, Оранжевый, Зеленый
LABELS = {
    1: '1 Закон (Только гравитация)', 
    2: '2 Закона (+ Трение)', 
    3: '3 Закона (+ Расталкивание дизъюнктов)'
}
MARKERS = {1: 'o', 2: 's', 3: '^'}
METHODS_TO_PLOT = ["RK4", "RK8", "DP8Adaptive"]

def plot_combined_grid(df):
    """Строит сетку 3x2 (Выбранные методы x [Раскрываемость, Асимптотика времени])"""
    fig, axes = plt.subplots(2, 3, figsize=(16, 10), sharex=True)
    fig.suptitle('Результаты Ablation Study: Влияние физических законов и ОДУ-решателей', fontsize=15, y=0.96, fontweight='bold')

    for col, method in enumerate(METHODS_TO_PLOT):
        m_df = df[df['Method'] == method]
        
        ax_succ = axes[0, col]
        ax_time = axes[1, col]
        
        for law in [1, 2, 3]:
            law_df = m_df[m_df['Law'] == law].sort_values('L')
            if law_df.empty: 
                continue
                
            # 1. Верхний ряд: Процент раскрываемости (успешный True SAT)
            ax_succ.plot(
                law_df['L'], law_df['Success_Rate'], 
                color=COLORS[law], marker=MARKERS[law], 
                linewidth=2, markersize=6, alpha=0.9
            )
            
            # 2. Нижний ряд: Асимптотика времени работы ОДУ с усами погрешности (Standard Deviation)
            ax_time.errorbar(
                law_df['L'], law_df['Time_Mean'], yerr=law_df['Time_Std'], 
                color=COLORS[law], marker=MARKERS[law], fmt='-o', 
                capsize=4, elinewidth=1.2, linewidth=2, markersize=6, alpha=0.9
            )

        ax_succ.set_title(f'Интегратор: {method}\nРаскрываемость КНФ', fontsize=11, pad=8)
        ax_time.set_title(f'Асимптотика времени', fontsize=11, pad=8)
        
        ax_succ.grid(True, linestyle=':', alpha=0.6)
        ax_time.grid(True, linestyle=':', alpha=0.6)
        
        # Подписи вертикальных осей только для крайнего левого ряда
        if col == 0:
            ax_succ.set_ylabel('Успешность решения (%)', fontsize=11)
            ax_time.set_ylabel('Время симуляции (сек)', fontsize=11)
            
        ax_time.set_xlabel('Размерность формулы (L)', fontsize=11)
        ax_time.set_xticks(sorted(df['L'].unique()))

    # Создание фейковых линий для сборки единой красивой легенды под графиком
    legend_elements = [
        plt.Line2D([0], [0], color=COLORS[l], marker=MARKERS[l], lw=2, label=LABELS[l]) 
        for l in [1, 2, 3]
    ]
    fig.legend(handles=legend_elements, loc='lower center', ncol=3, bbox_to_anchor=(0.5, 0.02), fontsize=11)
    
    plt.subplots_adjust(bottom=0.13, wspace=0.18, hspace=0.22)
    
    path = os.path.join(ROOT_DIR, "final_ablation_grid.png")
    plt.savefig(path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Научная панель (6 графиков) без Leapfrog сохранена в: {path}")

def plot_satisfied_clauses(df):
    """Строит сетку 1x3 (Средний процент выполненных дизъюнктов КНФ)"""
    fig, axes = plt.subplots(1, 3, figsize=(18, 5.5), sharey=True)
    fig.suptitle('Метрика качества сходимости ОДУ: Средний процент удовлетворенных дизъюнктов', fontsize=13, y=0.98, fontweight='bold')

    for col, method in enumerate(METHODS_TO_PLOT):
        m_df = df[df['Method'] == method]
        ax = axes[col]
        
        for law in [1, 2, 3]:
            law_df = m_df[m_df['Law'] == law].sort_values('L')
            if law_df.empty: 
                continue
            ax.plot(
                law_df['L'], law_df['Sat_Pct_Mean'], 
                color=COLORS[law], marker=MARKERS[law], 
                label=LABELS[law], linewidth=2, markersize=6
            )
            
        ax.set_title(f'Интегратор: {method}', fontsize=11)
        ax.set_xlabel('Размерность задачи (L)', fontsize=10)
        ax.grid(True, linestyle=':', alpha=0.6)
        ax.set_xticks(sorted(df['L'].unique()))

    axes[0].set_ylabel('Выполненные дизъюнкты (%)', fontsize=11)
    axes[0].set_ylim(50, 102) # Дизъюнкты обычно колеблются в верхней зоне
    
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc='lower center', ncol=3, bbox_to_anchor=(0.5, -0.06), fontsize=11)
    
    path = os.path.join(ROOT_DIR, "satisfied_clauses_grid.png")
    plt.savefig(path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"График метрики качества дизъюнктов сохранен в: {path}")

def main():
    if not os.path.exists(ABLATION_CSV):
        print(f"Ошибка: Файл {ABLATION_CSV} не найден! Выполните сбор данных через run_GA_ablation.py")
        return

    df = pd.read_csv(ABLATION_CSV)
    
    # Фильтруем входящий датасет от Leapfrog на всякий случай
    df_filtered = df[df['Method'].isin(METHODS_TO_PLOT)]
    
    plot_combined_grid(df_filtered)
    plot_satisfied_clauses(df_filtered)
    print("🏁 Отрисовка под твой стек численных методов полностью завершена.")

if __name__ == "__main__":
    main()