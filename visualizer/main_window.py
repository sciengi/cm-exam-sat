
from PySide6.QtWidgets import QMainWindow, QWidget, QHBoxLayout

from command_panel import CommandPanel
from main_panel import MainPanel
from plots_panel import PlotsPanel


class MainWindow(QMainWindow):
    
    # TODO: check Qt window types
    # TODO: add logging
    
    def __init__(self):
        super().__init__()
        
        self.setWindowTitle("Visualizer Prototype")
        self.resize(800, 600)

        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QHBoxLayout(central_widget)

        self.command_panel = CommandPanel()
        layout.addWidget(self.command_panel, stretch=1)

        self.main_panel = MainPanel()
        layout.addWidget(self.main_panel, stretch=4)
        
        self.plots_panel = PlotsPanel()
        layout.addWidget(self.plots_panel, stretch=2)
        
        self.command_panel.btn_load.clicked.connect(self.main_panel.viewport.update)
        
        self.main_panel.viewport.update()
