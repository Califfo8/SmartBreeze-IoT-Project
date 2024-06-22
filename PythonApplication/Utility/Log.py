import logging

class Resource(logging.Filter):
    def __init__(self, resource):
        super(Resource, self).__init__()
        self.resource = resource
    
    def filter(self, record):
        record.resource = self.resource
        return True

class Log:
    def __init__(self, module, resource):
        self.logger = logging.getLogger(module)
        self.logger.setLevel(logging.INFO)
        self.handler = logging.StreamHandler()
        self.formatter = logging.Formatter('%(levelname)s - %(module)s - %(resource)s - %(message)s')
        self.handler.setFormatter(self.formatter)
        self.logger.addHandler(self.handler)
        self.logger.addFilter(Resource(resource))
    
    def get_logger(self):
        return self.logger