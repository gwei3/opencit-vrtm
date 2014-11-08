MODULE_NAME='com.intel.rpcore.plugin'

# Nedded plugin config key
class KEY:
    RPCORE_IPADDR = 'RPCORE_IPADDR'   # RPCore IP address, rpclient uses this for connection
    RPCORE_PORT = 'RPCORE_PORT'     # RPCore listining port 
    DEBUG_LEVEL = 'DEBUG_LEVEL'


import load_config

from com.intel.rpcore.client.rpclient import RPClient

# path to the configuration file
FILE_PATH = '/etc/intel_rpcore_plugin.conf'

# load configuration file
CONFIG = load_config.load_configuration(FILE_PATH)
# set up logging
load_config.configure_logging(MODULE_NAME, (CONFIG[KEY.DEBUG_LEVEL]).upper())
# set up rpclient to coomunicate with RPCore
# set up  rpclient for rpcore communication
rpclient = RPClient(rpcore_ip_address=CONFIG[KEY.RPCORE_IPADDR], rpcore_port=int(CONFIG[KEY.RPCORE_PORT]))
