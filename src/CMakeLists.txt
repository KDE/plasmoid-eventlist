project(EVENTAPPLET)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(eventapplet_SRCS
    eventapplet.cpp
    eventmodel.cpp
    eventfiltermodel.cpp
    propertyfiltermodel.cpp
    eventtreeview.cpp
    eventitemdelegate.cpp
    korganizerappletutil.cpp
    generalconfig.cpp
    formatconfig.cpp
    headerdelegate.cpp
)

qt4_add_dbus_interface(eventapplet_SRCS org.kde.korganizer.Korganizer.xml korganizer_interface)
qt4_add_dbus_interface(eventapplet_SRCS org.kde.Korganizer.Calendar.xml calendar_interface)

kde4_add_ui_files(eventapplet_SRCS eventappletgeneralconfig.ui eventappletformatconfig.ui todoformatconfig.ui eventappletcolorconfig.ui)

kde4_add_plugin(plasma_applet_events ${eventapplet_SRCS})

set(MY_LIBRARIES
    ${KDE4_PLASMA_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_AKONADI_LIBS}
    ${KDE4_KCALCORE_LIBS}
    ${KDE4_KCALUTILS_LIBS}
    ${KDE4_KABC_LIBS}
    ${KDEPIMLIBS_AKONADI_KCAL_LIBS})

if (${KDE_VERSION} VERSION_LESS 4.5.0)
set(MY_LIBRARIES
    ${MY_LIBRARIES}
    ${KDE4_KIO_LIBS})
endif(${KDE_VERSION} VERSION_LESS 4.5.0)

target_link_libraries(plasma_applet_events ${MY_LIBRARIES})

install(TARGETS plasma_applet_events DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-applet-events.desktop DESTINATION ${SERVICES_INSTALL_DIR})
