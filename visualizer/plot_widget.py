
from PySide6.QtWidgets import QWidget, QVBoxLayout
import numpy as np
import matplotlib
matplotlib.use('QtAgg')
import matplotlib.pyplot as plt
plt.style.use('dark_background')  # TODO: fix app theme or add switching
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure


class PlotWidget(QWidget):
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        # TODO: add args to configure widget
        
        self.figure = Figure(figsize=(5, 4), dpi=100)
        self.canvas = FigureCanvas(self.figure)
        self.canvas.setFixedSize(500, 400)  # TODO: mind plot's geometry
        
        self.ax = self.figure.add_subplot()
        
        layout = QVBoxLayout(self)
        layout.addWidget(self.canvas)

        self.plot_data()

    def plot_data(self):
        
        # TODO: write template method that works with Qt part,
        #       get user callback to plot data
        
        self.ax.clear()
        
        x = np.linspace(0, 10, 100)
        y = np.sin(x)
        
        self.ax.plot(x, y, label="some plot", color="green", linewidth=2)
        self.ax.set_title("Matplotlib widget in PySide6")
        self.ax.set_xlabel("X")
        self.ax.set_ylabel("Y")
        self.ax.grid(True)
        self.ax.legend()
        
        self.canvas.draw()
        