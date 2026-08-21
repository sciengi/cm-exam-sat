
from PySide6.QtWidgets import QMainWindow, QWidget, QVBoxLayout
from qtconsole.rich_jupyter_widget import RichJupyterWidget
from qtconsole.inprocess import QtInProcessKernelManager


class ConsoleWindow(QMainWindow):
    
    def __init__(self, parent=None):
        super().__init__(parent)

        self.setWindowTitle("Console")
        self.resize(800, 600)

        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)
        
        # TODO: read about QtInProcessKernelManager and RichJupyterWidget:
        #       - what is a channelS? 
        #       - use widget without QtInProcessKernelManager?
        
        self.kernel_manager = QtInProcessKernelManager()   
        self.kernel_manager.start_kernel()                
        
        self.kernel_client = self.kernel_manager.client()
        self.kernel_client.start_channels()

        self.console = RichJupyterWidget()
        self.console.kernel_manager = self.kernel_manager
        self.console.kernel_client  = self.kernel_client
        self.console.set_default_style('linux')
        self.console.syntax_style = 'monokai'
        self.console.font_family = 'Consolas'
        self.console.font_size = 14

        # TODO(core): push PlotPanel and events to shell, how to sync shell instance with app?
        self.kernel_manager.kernel.shell.push({
            "var": "PUSHED"
        })

        layout.addWidget(self.console)

    def closeEvent(self, event):
        self.kernel_client.stop_channels()
        self.kernel_manager.shutdown_kernel()
        event.accept()
        