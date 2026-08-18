
from PySide6.QtWidgets import QTabWidget

from viewport_widget import ViewportWidget
from console_widget import ConsoleWidget


class MainPanel(QTabWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        
        self.setTabsClosable(False)
        
        self.viewport = ViewportWidget()
        self.console  = ConsoleWidget()
        
        self.addTab(self.viewport, 'Viewport')
        self.addTab(self.console, 'Console')