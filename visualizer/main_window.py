
from PySide6.QtCore import Slot
from PySide6.QtWidgets import QMainWindow, QWidget, QHBoxLayout, QTabWidget

from left_panel import LeftPanel
from task_panel import TaskPanel
from source import FileSource
from events import EventResolver, EventDispatcher

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
        self.left_panel.control.step_requested.connect(self.step)
        layout.addWidget(self.left_panel, stretch=1)

        self.tasks = QTabWidget()
        self.tasks.setTabsClosable(True)
        self.tasks.tabCloseRequested.connect(self.close_task)
        
        layout.addWidget(self.tasks, stretch=3)
        
        # TODO: block btns in ControlPanel when no source of task
        
        # TODO: think who must contain Event*
        self.event_resolver = EventResolver()
        self.event_dispatcher = EventDispatcher()
        
        # TIP:
        # self.event_dispatcher\
        #     .add_rule(('SYSTEM','status'), lambda event: ...)\
        
        # TODO: add to EventDispatcher event_id placeholder ANY scope and any subject
        # TODO: think about dynamic rule addition when new recipient created,
        #       add signal in all classes or use upstream panels, ...
        
        # TODO: add Slot to all 
        
    @Slot(str)
    def new_task(self, filename: str):
        # TODO: write tab name generation by filename
        # TODO(future): add other sources
        task = TaskPanel(FileSource(filename))  # TODO: check filename
        task.event_subscribe_requested.connect(self.subscribe_to_event)
        # meta_method = QMetaMethod.fromSignal(task.event_subscribe_requested)
        # is_connected = task.isSignalConnected(meta_method)
        task._event_subscribe_requested_emit()
        self.tasks.addTab(task, filename)
        
    def close_task(self, index):
        widget = self.tasks.widget(index)
        if widget is not None:
            self.tasks.removeTab(index)
            widget.deleteLater()
           
    @Slot(tuple, object) 
    def subscribe_to_event(self, event_id, recipent_method):
        self.event_dispatcher.add_rule(event_id, recipent_method)
    
    # def unsubscribe_to_event(self, event_id, recipent_method): pass  # TODO: ???
    
    @Slot()
    def step(self):  # TODO: change name
        task = self.tasks.currentWidget()
        if task is None:
            return  # TODO: add warn
        
        try: # TODO: refactor
            record = task.source.get_record()  
        except EOFError as e:
            print(e)
            return
        
        event = self.event_resolver.resolve(record)
        self.event_dispatcher.dispatch(event)
        
        print('STEP EventDispatcher routes:')
        print(self.event_dispatcher._route_table)        
        
        
        
        
        