add_executable(idleTest idle_test.cpp)
target_link_libraries(idleTest Qt::Gui Qt::Test KF5::IdleTime KF5::WaylandClient)
kwaylandtest(idleTest)
ecm_mark_as_test(idleTest)
