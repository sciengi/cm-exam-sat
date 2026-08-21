
import re
import numpy as np
from weakref import WeakMethod

# TIP: event is a dict with keys 'scope', 'subject' and other for data


class EventResolver:
    ''' Cast event-record from source to dict with preprocessed fields '''
    
    # TIP: if you need preprocess event fields
    #      - add to __init__ our function
    #      - add to self._conv new (event ID, function) pair
    
    def __init__(self):
        pattern = r'(?P<scope>\w+)\((?P<subject>\w+)\):\s*(?P<msg>.*)'
        self._base_pattern = re.compile(pattern)
        
        def _sysdata_conv(event) -> None:
            event['data'] = np.array([float(v) for v in event['msg'].split()])
            del event['msg']
        
        # TODO: connect event type and conversion on it, 
        #       check pydantic package
        
        self._conv = {
            ('SYSTEM', 'data'): _sysdata_conv
        }
    
    # TODO(extra): add ability to add pattern and conversion in runtime
    #              
    # def add_pattern(self) -> None: pass  

    def _convert_types(self, event: dict) -> None:
        event_id = (event['scope'], event['subject'])
        if event_id in self._conv:
            self._conv[event_id](event)

    def resolve(self, record: str) -> dict:
        result = re.match(self._base_pattern, record)
        
        if result is not None:
            event = result.groupdict()
            self._convert_types(event)
        else:
            event = {
                'scope': 'VISUALIZER',
                'subject': 'task_source',
                'msg': f'unknown record {record}'
            }
            
        return event


class EventDispatcher:
    ''' Dispatch event to recipents '''
        
    def __init__(self):
        self._route_table = {}
    
    def _get_recipients(self, event_id) -> list:
        return self._route_table.get(event_id, [])
    
    def add_rule(self, event_id, recipent_method) -> self:  # DEV: chain rule
        # TODO: add placeholders ANY scope and any subject
        # TODO: refactor or pass
        #     self._get_recipients(event_id).append(WeakMethod(recipent_method)) 
        # changed to:
        if event_id in self._route_table:
            self._route_table[event_id].append(WeakMethod(recipent_method))
        else:
            self._route_table[event_id] = [WeakMethod(recipent_method)]
        
        return self
    
    def dispatch(self, event: dict) -> None:
        event_id = (event['scope'], event['subject'])
        still_alive = []
        for recipient in self._get_recipients(event_id):
            if recipient is not None:
                recipient()(event)
                still_alive.append(recipient)
            
        self._route_table[event_id] = still_alive
