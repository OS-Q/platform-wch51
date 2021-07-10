from platform import system
from platformio.managers.platform import PlatformBase

class Wch51Platform(PlatformBase):
    def configure_default_packages(self, variables, target):
        return PlatformBase.configure_default_packages(self, variables,target)
