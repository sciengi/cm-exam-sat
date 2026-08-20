
from PySide6.QtWidgets import QWidget, QHBoxLayout

from viewport_widget import ViewportWidget
from plots_panel import PlotsPanel
from source import Source

class TaskPanel(QWidget):
    def __init__(self, source: Source, parent=None):
        super().__init__(parent)
        
        self.source = source
        
        layout = QHBoxLayout(self)
                
        self.viewport = ViewportWidget()
        layout.addWidget(self.viewport, stretch=3)
        
        self.plots = PlotsPanel()
        layout.addWidget(self.plots, stretch=1)
        