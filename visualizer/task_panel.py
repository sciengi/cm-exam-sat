
from PySide6.QtWidgets import QWidget, QHBoxLayout

from viewport_widget import ViewportWidget
from plots_panel import PlotsPanel


class TaskPanel(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        
        # TODO: specify task source here
        
        layout = QHBoxLayout(self)
                
        self.viewport = ViewportWidget()
        layout.addWidget(self.viewport, stretch=3)
        
        self.plots = PlotsPanel()
        layout.addWidget(self.plots, stretch=1)
        