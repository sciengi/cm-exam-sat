
from PySide6.QtCore import Signal
from PySide6.QtWidgets import QWidget, QHBoxLayout, QVBoxLayout

from source import Source
from viewport_widget import ViewportWidget
from event_viewer_widget import EventViewerWidget
from plots_panel import PlotsPanel


class TaskPanel(QWidget):
    
    event_subscribe_requested = Signal(tuple, object)
    
    def __init__(self, source: Source, parent=None):
        super().__init__(parent)
        
        self.source = source
        
        layout = QHBoxLayout(self)
        
        box = QWidget()
        box_layout = QVBoxLayout(box)
        layout.addWidget(box, stretch=3)
        
        self.viewport = ViewportWidget()
        box_layout.addWidget(self.viewport, stretch=5)
        
        self.event_viewer = EventViewerWidget()
        box_layout.addWidget(self.event_viewer, stretch=1)
        
        self.plots = PlotsPanel()
        # self.event_subscibe_requested.emit(('SYSTEM', 'data'), self.plots.update)  # TODO:
        # self.event_subscibe_requested.emit(('METRIC', 'any'), self.plots.update)  # TODO:
        layout.addWidget(self.plots, stretch=1)
        
    def _event_subscribe_requested_emit(self):  # TODO: think how to make reliably (BUG with .connecy in MainWindow)
        self.event_subscribe_requested.emit(('SYSTEM', 'data'), self.viewport.update)
        
        self.event_subscribe_requested.emit(('SYSTEM',    'data'),       self.event_viewer.update)
        self.event_subscribe_requested.emit(('SYSTEM',    'status'),     self.event_viewer.update)
        self.event_subscribe_requested.emit(('TASK',      'setup'),      self.event_viewer.update)
        self.event_subscribe_requested.emit(('CONSTRAIN', 'step_limit'), self.event_viewer.update)
        