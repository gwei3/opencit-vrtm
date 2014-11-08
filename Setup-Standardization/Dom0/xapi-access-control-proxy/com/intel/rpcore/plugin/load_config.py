import json
import logging
import logging.handlers
import os
from com.intel.rpcore.plugin import MODULE_NAME,KEY
from exception import RPPluginEnvConfigNotFound
logging_level = {'DEBUG' : logging.DEBUG,
                 'WARNING' : logging.WARNING,
                 'ERROR' : logging.ERROR}

def configure_logging(name, log_level='DEBUG'):
    log = logging.getLogger()
    log.setLevel(logging_level[log_level] )
    
    #sys_handler = logging.handlers.SysLogHandler('/dev/log')
    #sys_handler.setLevel(logging_level[log_level])
    handler = logging.FileHandler("access-control.log","w")  
    handler.setLevel(logging_level[log_level])
    formatter = logging.Formatter('%s: %%(levelname)-8s %%(message)s' % name)
    
    handler.setFormatter(formatter)
    log.addHandler(handler)

def load_configuration(config_file_path):
    CONFIG = {}
    try:
        conf_file = file(config_file_path)
        config_dct = json.load(conf_file)
        logging.debug(config_dct)
        conf_file.close()
    except TypeError:
        configure_logging(MODULE_NAME)
        logging.error('Configuration file is missing at location %s' % file_path)
        raise RPPluginEnvConfigNotFound('Configuration file is missing at location %s' % file_path)
    return config_dct    

if __name__ == '__main__':
    configure_logging(MODULE_NAME)
    load_configuration('/etc/intel_rpcore_plugin.conf')
