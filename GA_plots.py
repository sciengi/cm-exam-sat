import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Загрузка данных
file_path = 'genetic_research_results.csv'
df = pd.read_csv(file_path)

# 1. Группировка и расчет среднего для каждого L
df_grouped = df.groupby('L').mean().reset_index()

# 2. Построение графиков
fig, axes = plt.subplots(2, 2, figsize=(12, 10))
fig.suptitle('Зависимость физических параметров от размерности L (на основе ГА)', fontsize=16)

params = [('c_att', 'Притяжение клонов'), 
          ('c_opp', 'Отталкивание антиподов'), 
          ('c_clause', 'Расталкивание скобок'), 
          ('gamma', 'Коэффициент трения')]

for i, (param, label) in enumerate(params):
    ax = axes[i // 2, i % 2]
    sns.lineplot(data=df_grouped, x='L', y=param, marker='o', ax=ax, color='teal')
    ax.set_title(label)
    ax.grid(True, linestyle='--')

plt.tight_layout(rect=[0, 0.03, 1, 0.95])
plt.savefig('genetic_analysis_plots.png')
plt.show()

# Вывод сводной таблицы для отчета
print("Средние параметры по размерностям L:")
print(df_grouped[['L', 'c_att', 'c_opp', 'c_clause', 'gamma']].to_string(index=False))