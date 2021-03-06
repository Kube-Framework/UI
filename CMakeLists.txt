kube_add_library(UI
    SOURCES
        # Animation.cpp
        # Animation.hpp
        App.cpp
        App.hpp
        Base.cpp
        Base.hpp
        Base.ipp
        Components.hpp
        # Curve.hpp
        # Curve.ipp
        Events.hpp
        EventQueue.hpp
        EventQueue.ipp
        EventSystem.cpp
        EventSystem.hpp
        EventSystem.ipp
        Item.cpp
        Item.hpp
        Item.ipp
        Font.cpp
        Font.hpp
        Font.ipp
        FontManager.cpp
        FontManager.hpp
        Painter.cpp
        Painter.hpp
        Painter.ipp
        PresentPipeline.hpp
        PresentSystem.cpp
        PresentSystem.hpp
        Primitive.hpp
        PrimitiveProcessor.hpp
        RectangleProcessor.hpp
        RectangleProcessor.cpp
        Renderer.cpp
        Renderer.hpp
        Renderer.ipp
        RendererProcessor.hpp
        Sprite.cpp
        Sprite.hpp
        Sprite.ipp
        SpriteManager.cpp
        SpriteManager.hpp
        TextProcessor.hpp
        TextProcessor.cpp
        TraverseContext.cpp
        TraverseContext.hpp
        UIObject.cpp
        UIObject.hpp
        UISystem.cpp
        UISystem.hpp
        UISystem.ipp

    SHADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Primitive.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Primitive.frag
        ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Rectangle.comp
        ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Text.comp

    LIBRARIES
        ECS
        GPU
        IO
        stb
        FreeTypeStatic
)