
from PySide6.QtCore import Signal
from PySide6.QtWidgets import QWidget, QVBoxLayout, QPushButton, QFileDialog

from console_window import ConsoleWindow


class ControlWidget(QWidget):
    
    new_task_selected = Signal(str)
    step_requested = Signal() 
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        self.console_window = None
        
        layout = QVBoxLayout(self)
        
        # TODO: add functionallity to each button
        # TODO: mind about buttons state (no source - all disabled, etc)
        
        self.btn_new = QPushButton('New Task')
        self.btn_new.clicked.connect(self.browse_task)
        layout.addWidget(self.btn_new)
        
        self.btn_console = QPushButton('Console')
        self.btn_console.clicked.connect(self.open_console)
        layout.addWidget(self.btn_console)
        
        # TODO: add task control widget, to dispatch events in one place
        
        self.btn_step_forward = QPushButton('Step')
        self.btn_step_forward.clicked.connect(lambda: self.step_requested.emit())
        layout.addWidget(self.btn_step_forward)
        
        self.btn_step_backward = QPushButton('Back')
        self.btn_step_backward.setEnabled(False)
        layout.addWidget(self.btn_step_backward)
        
        self.btn_goto = QPushButton('Goto')
        self.btn_goto.setEnabled(False)
        layout.addWidget(self.btn_goto)

    def open_console(self):
        if self.console_window is None or not self.console_window.isVisible():
            self.console_window = ConsoleWindow()
            self.console_window.show()
            
    def browse_task(self):
        filepath, _ = QFileDialog.getOpenFileName(self, caption='', dir='')  # TODO: setup filter and dir
        if filepath:
            self.new_task_selected.emit(filepath)
            