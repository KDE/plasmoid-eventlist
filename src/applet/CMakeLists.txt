project(EVENTAPPLET)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(eventapplet_SRCS
    eventapplet.cpp
    eventmodel.cpp
    eventtreeview.cpp
    eventitemdelegate.cpp
    korganizerappletutil.cpp
)

qt4_add_dbus_interface(eventapplet_SRCS org.kde.korganizer.Korganizer.xml korganizer_interface)
qt4_add_dbus_interface(eventapplet_SRCS org.kde.Korganizer.Calendar.xml calendar_interface)

kde4_add_ui_files(eventapplet_SRCS eventappletformatconfig.ui eventappletcolorconfig.ui)

kde4_add_plugin(plasma_applet_events ${eventapplet_SRCS})
target_link_libraries(plasma_applet_events
                      ${KDE4_PLASMA_LIBS}
                      ${KDE4_KDEUI_LIBS}
                      ${KDE4_AKONADI_LIBS}
                      ${KDE4_KCAL_LIBS}
                      ${KDE4_KABC_LIBS}
                      ${KDEPIMLIBS_AKONADI_KCAL_LIBS})

install(TARGETS plasma_applet_events DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-applet-events.desktop DESTINATION ${SERVICES_INSTALL_DIR})
