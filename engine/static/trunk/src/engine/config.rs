use lowlevel::math::misc::ScreenSpaceScaleMode;

pub struct EngineConfig {
    pub(crate) target_tickrate: Option<u32>,
    pub(crate) target_framerate: Option<u32>,
    pub(crate) load_modules: Vec<String>,
    pub(crate) render_backends: Vec<String>,
    pub(crate) scale_mode: ScreenSpaceScaleMode
}

impl Default for EngineConfig {
    fn default() -> Self {
        EngineConfig {
            target_tickrate: None,
            target_framerate: None,
            load_modules: vec![],
            render_backends: vec![],
            scale_mode: ScreenSpaceScaleMode::NormalizeMinDimension
        }
    }
}

impl EngineConfig {
    pub fn load_client_config(config_namespace: &str) {
        //TODO
    }

    /**
     * \brief Sets the target tickrate of the engine.
     *
     * When performance allows, the engine will sleep between updates to
     * enforce this limit. Set to 0 to disable tickrate targeting.
     *
     * \param target_tickrate The new target tickrate in updates/second.
     *
     * \attention This is independent from the target framerate, which controls
     *            how frequently frames are rendered.
     */
    pub fn set_target_tickrate(&mut self, target_tickrate: u32) {
        self.target_tickrate = Some(target_tickrate);
    }

    /**
     * \brief Sets the target framerate of the engine.
     *
     * When performance allows, the engine will sleep between frames to
     * enforce this limit. Set to 0 to disable framerate targeting.
     *
     * \param target_framerate The new target framerate in frames/second.
     *
     * \attention This is independent from the target tickrate, which controls
     *            how frequently the game logic routine is called.
     */
    pub fn set_target_framerate(&mut self, target_framerate: u32) {
        self.target_framerate = Some(target_framerate);
    }

    /**
     * \brief Sets the modules to load on engine initialization.
     *
     * If any provided module or any its respective dependencies cannot be
     * loaded, engine initialization will fail.
     *
     * \param module_list The IDs of the modules to load on engine init.
     */
    pub fn set_load_modules(&mut self, module_list: &[&str]) {
        self.load_modules = module_list.iter().map(|&s| s.into()).collect();
    }

    /**
     * \brief Returns a list of graphics backends available for use on the
     *        current platform.
     *
     * \return The available graphics backends.
     */
    pub fn get_available_render_backends(&self) -> Vec<String> {
        return Vec::new(); //TODO
    }

    /**
     * \brief Returns an ordered list of IDs of preferred render backends as
     *        specified by the client.
     * 
     * \return An ordered list of preferred render backend IDs.
     */
    pub fn get_preferred_render_backends<'a>(&'a self) -> &'a Vec<String> {
        &self.render_backends
    }

    /**
     * \brief Sets the graphics backend to be used for rendering.
     *
     * \param backend A list of render backends to use in order of preference.
     *
     * \remark This option is treated like a "hint" and will not be honored in
     *          the event that the preferred backend is not available, either
     *          due to a missing implementation or lack of hardware support. If
     *          none of the specified backends can be used, the OpenGL backend
     *          will be used as the default fallback.
     */
    pub fn set_render_backends(&mut self, backends: &[&str]) {
        self.render_backends = backends.iter().map(|&s| s.into()).collect();
    }

    /**
     * \brief Sets the graphics backend to be used for rendering.
     *
     * \param backend The preferred backend to use.
     *
     * \remark This option is treated like a "hint" and will not be honored in
     *          the event that the preferred backend is not available, either
     *          due to a missing implementation or lack of hardware support. If
     *          none of the specified backends can be used, the OpenGL backend
     *          will be used as the default fallback.
     */
    pub fn set_render_backend(&mut self, backend: &str) {
        self.render_backends = vec![backend.to_string()];
    }

    /**
     * \brief Returns the currently configured scale mode for the screen space.
     *
     * This controls how the view matrix passed to shader programs while
     * rendering the screen is computed.
     * 
     * \return The current screen space scale mode.
     *
     * \sa ScreenSpaceScaleMode
     */
    pub fn get_screen_space_scale_mode(&self) -> ScreenSpaceScaleMode {
        self.scale_mode
    }

    /**
     * \brief Sets the screen space scale mode
     *
     * The scale mode used to compute the view matrix passed to shader programs
     * while rendering objects to the screen.
     *
     * If this value is not provided, it will default
     * ScreenSpaceScaleMode::NormalizeMinDimension.
     *
     * \param scale_mode The screen space scale mode to use.
     *
     * \sa ScreenSpaceScaleMode
     */
    pub fn set_screen_space_scale_mode(&mut self, scale_mode: ScreenSpaceScaleMode) {
        self.scale_mode = scale_mode;
    }
}
