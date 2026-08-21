
from PySide6.QtWidgets import QWidget, QVBoxLayout

from plot_widget import PlotWidget


class PlotsPanel(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        
        layout = QVBoxLayout(self)
        
        # TODO: add routine to add PlotWidgets (mb by interface)
        # TODO: add scrolling
        # TODO: how to work with plots sizes?
        
        self.stub = PlotWidget()
        layout.addWidget(self.stub)
        