
from PySide6.QtWidgets import QWidget, QScrollArea, QVBoxLayout, QLabel 


class EventViewerWidget(QWidget):
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        layout = QVBoxLayout(self)
        
        self.label = QLabel('Event Viewer')
        layout.addWidget(self.label)
        
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        layout.addWidget(scroll)
        
        self.event_fields = QWidget()
        self.event_fields_layout = QVBoxLayout(self.event_fields)
        self.event_fields_widgets = []
        
        scroll.setWidget(self.event_fields)
        
        # TODO: add filter capability to event data, event them selves,
        #       add signature for functions
        #       add runtime access to filters
        def sysdata_filter(key, value):
            if key == 'data':
                return '<...>'
            return value
        
        self._filter = {
            ('SYSTEM', 'data'): sysdata_filter
        }
    
    def _get_event_fields_filter(self, event_id):
        return self._filter.get(event_id, lambda key, value: value)
        
    def update(self, event: dict):
        
        for field_widget in self.event_fields_widgets:
            self.event_fields_layout.removeWidget(field_widget)
            field_widget.deleteLater()  # TODO: maybe reuse already existing widgets?
            
        self.event_fields_widgets.clear()
        
        event_id = (event['scope'], event['subject'])  # TODO: add to event module, saw in several places
        field_filter = self._get_event_fields_filter(event_id)
        
        for key, val in event.items():
            label = QLabel(f'{key}=\'{field_filter(key, val)}\'')
            self.event_fields_layout.addWidget(label)
            self.event_fields_widgets.append(label)
            