class RPPluginException(Exception):
    """
    Base class for all exception of rpcore plugin
    """
    def __init__(self, *args):
        Exception.__init__(self, *args)
        
class RPPluginEnvConfigNotFound(RPPluginException):
    """
    raise if /usr/etc/intel_rp_plugin.conf file not available or accessible.
    """
    def __init__(self, *args):
        RPPluginException.__init__(self, *args)

class VMLaunchNotAllowed(RPPluginException):
    """
    raise if /usr/etc/intel_rp_plugin.conf file not available or accessible.
    """
    def __init__(self, *args):
        RPPluginException.__init__(self, *args)

class AccessControlException(RPPluginException):
    """
    raise if /usr/etc/intel_rp_plugin.conf file not available or accessible.
    """
    def __init__(self, *args):
        RPPluginException.__init__(self, *args)

