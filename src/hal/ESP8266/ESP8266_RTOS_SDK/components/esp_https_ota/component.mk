COMPONENT_SRCDIRS :=
COMPONENT_ADD_INCLUDEDIRS :=

ifdef CONFIG_SSL_USING_MBEDTLS 
COMPONENT_SRCDIRS := src
COMPONENT_ADD_INCLUDEDIRS := include
endif

ifdef CONFIG_SSL_USING_WOLFSSL
COMPONENT_SRCDIRS := src
COMPONENT_ADD_INCLUDEDIRS := include
endif