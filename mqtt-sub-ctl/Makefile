include $(TOPDIR)/rules.mk

PKG_NAME:=mqtt_sub_ctl
PKG_RELEASE:=1
PKG_VERSION:=0.1.0
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE

include $(INCLUDE_DIR)/package.mk

define Package/mqtt_sub_ctl
	CATEGORY:=Utilities
	TITLE:=mqtt_sub_ctl
	DEPENDS:= +libuci +libjson-c 
endef

define Package/mqtt_sub_ctl/description
	Controller for MQTT subscriber daemon program
endef

define Package/mqtt_sub_ctl/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mqtt_sub_ctl $(1)/usr/sbin
endef

$(eval $(call BuildPackage,mqtt_sub_ctl))
