project(EVENTAPPLET)

set(eventapplet_SRCS
    eventapplet.cpp
    eventmodel.cpp
    eventitemdelegate.cpp
    korganizerappletutil.cpp
)

qt4_add_dbus_interface(eventapplet_SRCS org.kde.korganizer.Korganizer.xml korganizer_interface)
qt4_add_dbus_interface(eventapplet_SRCS org.kde.Korganizer.Calendar.xml calendar_interface)

kde4_add_ui_files(eventapplet_SRCS eventappletformatconfig.ui eventappletcolorconfig.ui)

kde4_add_plugin(eventapplet ${eventapplet_SRCS})
target_link_libraries(eventapplet ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS})

install(TARGETS eventapplet DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES eventapplet-default.desktop DESTINATION ${SERVICES_INSTALL_DIR})
