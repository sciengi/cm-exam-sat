
from PySide6.QtWidgets import QWidget, QVBoxLayout

from control_widget import ControlWidget


class LeftPanel(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        
        layout = QVBoxLayout(self)

        self.control = ControlWidget()
        layout.addWidget(self.control, stretch=1)

        self.stub = QWidget()
        layout.addWidget(self.stub, stretch=1)
        