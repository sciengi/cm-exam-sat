
from PySide6.QtWidgets import QMainWindow, QWidget, QHBoxLayout, QTabWidget

from left_panel import LeftPanel
from task_panel import TaskPanel

class MainWindow(QMainWindow):
    
    # TODO: check Qt window types
    # TODO: add logging
    
    def __init__(self):
        super().__init__()
        
        self.setWindowTitle('Visualizer Prototype')
        self.resize(800, 600)

        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QHBoxLayout(central_widget)

        self.left_panel = LeftPanel()
        self.left_panel.control.new_task_selected.connect(self.new_task)
        layout.addWidget(self.left_panel, stretch=1)

        self.tasks = QTabWidget()
        self.tasks.setTabsClosable(True)
        self.tasks.tabCloseRequested.connect(self.close_task)
        layout.addWidget(self.tasks, stretch=3)
        
    def new_task(self, filename: str):
        # TODO: write tab name generation by filename
        self.tasks.addTab(TaskPanel(), filename)
        
    def close_task(self, index):
        widget = self.tasks.widget(index)
        if widget is not None:
            self.tasks.removeTab(index)
            widget.deleteLater()