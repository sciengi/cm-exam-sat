import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import cm
from scipy.interpolate import griddata

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
MASTER_CSV = os.path.join(ROOT_DIR, "grid_search_master.csv")

def main():
    if not os.path.exists(MASTER_CSV):
        print(f"❌ Ошибка: Файл {MASTER_CSV} не найден!")
        return

    df = pd.read_csv(MASTER_CSV)
    if df.empty:
        return

    print(f"[I] Обработка {len(df)} строк мастер-файла для построения поверхностей...")

    # Фиксируем лучшую фоновую пару (c_clause, gamma)
    best_fixed = df.groupby(["c_clause", "gamma"])["Success_Rate"].mean().idxmax()
    fixed_clause, fixed_gamma = best_fixed

    sub_df = df[(df["c_clause"] == fixed_clause) & (df["gamma"] == fixed_gamma)]
    
    # Агрегируем плоскость сил c_att и c_opp
    pivot_df = sub_df.pivot_table(values="Success_Rate", index="c_opp", columns="c_att", aggfunc=np.mean)

    # Координаты сетки
    x_raw = pivot_df.columns.values  # c_att
    y_raw = pivot_df.index.values    # c_opp
    X, Y = np.meshgrid(x_raw, y_raw)
    Z = pivot_df.values

    # ================= ИЗОБРАЖЕНИЕ 1: СГЛАЖЕННАЯ 3D ПОВЕРХНОСТЬ =================
    fig = plt.figure(figsize=(11, 8))
    ax = fig.add_subplot(111, projection='3d')
    
    surf = ax.plot_surface(
        X, Y, Z, 
        cmap=cm.coolwarm, 
        linewidth=0.5, 
        antialiased=True, 
        alpha=0.92,
        edgecolor='k'
    )
    
    ax.set_title(f'Гладкая поверхность эффективности метода Матиясевича (Шаг сил: 0.1)\n(Фиксация: $c_{{clause}}={fixed_clause}$, $\\gamma={fixed_gamma}$)', fontsize=11, pad=15)
    ax.set_xlabel('Сила притяжения клонов ($c_{att}$)', fontsize=10, labelpad=8)
    ax.set_ylabel('Сила отталкивания антиподов ($c_{opp}$)', fontsize=10, labelpad=8)
    ax.set_zlabel('Успешность раскрываемости SAT (%)', fontsize=10, labelpad=8)
    
    fig.colorbar(surf, ax=ax, shrink=0.5, aspect=12, label='Процент выполненных КНФ (%)')
    ax.view_init(elev=30, azim=-135)
    
    plot3d_path = os.path.join(ROOT_DIR, "grid_3d_surface_dense.png")
    plt.savefig(plot3d_path, dpi=300, bbox_inches='tight')
    plt.close()

    # ================= ИЗОБРАЖЕНИЕ 2: ИНТЕРПОЛИРОВАННЫЙ HIGH-RES HEATMAP =================
    plt.figure(figsize=(8.5, 6.5))
    
    # Создаем сверхплотную сетку для красивой кубической интерполяции градиентов
    x_dense = np.linspace(x_raw.min(), x_raw.max(), 300)
    y_dense = np.linspace(y_raw.min(), y_raw.max(), 300)
    X_dense, Y_dense = np.meshgrid(x_dense, y_dense)
    
    # Преобразуем исходную матрицу в плоские массивы точек для SciPy
    points = np.array(list(itertools.product(y_raw, x_raw)))
    values = Z.flatten()
    
    # Математическая кубическая интерполяция вычмат-поверхности
    Z_dense = griddata(points, values, (Y_dense, X_dense), method='cubic')

    heatmap = plt.imshow(
        Z_dense, 
        extent=[x_raw.min(), x_raw.max(), y_raw.min(), y_raw.max()],
        origin='lower', 
        cmap='coolwarm', 
        aspect='auto'
    )
    
    plt.colorbar(heatmap, label='Средний успех решения формул (%)')
    plt.title(f'Топографическая карта резонанса сил $c_{{att}}$ и $c_{{opp}}$ (Кубический срез)\n(При $c_{{clause}}={fixed_clause}$, $\\gamma={fixed_gamma}$)', fontsize=11)
    plt.xlabel('Коэффициент притяжения клонов ($c_{att}$)', fontsize=10)
    plt.ylabel('Коэффициент отталкивания антиподов ($c_{opp}$)', fontsize=10)
    plt.grid(True, linestyle=':', alpha=0.4)
    
    heatmap_path = os.path.join(ROOT_DIR, "grid_2d_heatmap_dense.png")
    plt.savefig(heatmap_path, dpi=300, bbox_inches='tight')
    plt.close()

    print(f"📈 Сглаженные графики сохранены в корне проекта: 'grid_3d_surface_dense.png' и 'grid_2d_heatmap_dense.png'")

if __name__ == "__main__":
    main()