
from PySide6.QtWidgets import QWidget, QVBoxLayout, QPushButton


class CommandPanel(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        
        layout = QVBoxLayout(self)
        
        # TODO: add functionallity to each button
        # TODO: mind about buttons state (no source - all disabled, etc)
        
        self.btn_load = QPushButton('Source')
        # self.btn_load.setEnabled(False)
        layout.addWidget(self.btn_load)
        
        self.btn_step_forward = QPushButton('Step')
        self.btn_load.setEnabled(False)
        layout.addWidget(self.btn_step_forward)
        
        self.btn_step_backward = QPushButton('Back')
        self.btn_load.setEnabled(False)
        layout.addWidget(self.btn_step_backward)
        
        self.btn_goto = QPushButton('Goto')
        self.btn_load.setEnabled(False)
        layout.addWidget(self.btn_goto)
