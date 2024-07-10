import logging

class Resource(logging.Filter):
    def __init__(self, resource):
        super(Resource, self).__init__()
        self.resource = resource
    
    def filter(self, record):
        record.resource = self.resource
        return True
    
    def update_resource(self, resource):
        self.resource = resource

class Log:
    def __init__(self, module, resource):
        self.filter = Resource(resource)
        self.logger = logging.getLogger(module)
        self.logger.setLevel(logging.INFO)
        self.handler = logging.StreamHandler()
        self.formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(module)s - %(resource)s - %(message)s', datefmt='%Y-%m-%d %H:%M:%S' )
        self.handler.setFormatter(self.formatter)
        self.logger.addHandler(self.handler)
        self.logger.addFilter(self.filter)
    
    def set_level(self, level):
        self.logger.setLevel(level)
    
    def set_resource(self, resource):
        self.filter.update_resource(resource)
    
    def get_logger(self):
        return self.logger