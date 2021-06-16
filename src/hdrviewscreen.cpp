//
// Copyright (C) Wojciech Jarosz <wjarosz@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE.txt file.
//

#include "hdrviewscreen.h"
#include "commandhistory.h"
#include "common.h"
#include "editimagepanel.h"
#include "hdrcolorpicker.h"
#include "hdrimageview.h"
#include "helpwindow.h"
#include "imagelistpanel.h"
#include "popupmenu.h"
#include "xpuimage.h"
#include <iostream>
#include <nanogui/opengl.h>
#define NOMINMAX
#include <spdlog/spdlog.h>
#include <thread>
#include <tinydir.h>

#include "tool.h"

using namespace std;

HDRViewScreen::HDRViewScreen(float exposure, float gamma, bool sRGB, bool dither, vector<string> args) :
    Screen(nanogui::Vector2i(800, 600), "HDRView", true)
{
    set_background(Color(0.23f, 1.0f));

    auto theme                     = new Theme(m_nvg_context);
    theme->m_standard_font_size    = 16;
    theme->m_button_font_size      = 15;
    theme->m_text_box_font_size    = 14;
    theme->m_window_corner_radius  = 4;
    theme->m_window_fill_unfocused = Color(40, 250);
    theme->m_window_fill_focused   = Color(45, 250);
    set_theme(theme);

    auto panel_theme                       = new Theme(m_nvg_context);
    panel_theme                            = new Theme(m_nvg_context);
    panel_theme->m_standard_font_size      = 16;
    panel_theme->m_button_font_size        = 15;
    panel_theme->m_text_box_font_size      = 14;
    panel_theme->m_window_corner_radius    = 0;
    panel_theme->m_window_fill_unfocused   = Color(50, 255);
    panel_theme->m_window_fill_focused     = Color(52, 255);
    panel_theme->m_window_header_height    = 0;
    panel_theme->m_window_drop_shadow_size = 0;

    auto flat_theme                             = new Theme(m_nvg_context);
    flat_theme                                  = new Theme(m_nvg_context);
    flat_theme->m_standard_font_size            = 16;
    flat_theme->m_button_font_size              = 15;
    flat_theme->m_text_box_font_size            = 14;
    flat_theme->m_window_corner_radius          = 0;
    flat_theme->m_window_fill_unfocused         = Color(50, 255);
    flat_theme->m_window_fill_focused           = Color(52, 255);
    flat_theme->m_window_header_height          = 0;
    flat_theme->m_window_drop_shadow_size       = 0;
    flat_theme->m_button_corner_radius          = 4;
    flat_theme->m_border_light                  = flat_theme->m_transparent;
    flat_theme->m_border_dark                   = flat_theme->m_transparent;
    flat_theme->m_button_gradient_top_focused   = flat_theme->m_transparent;
    flat_theme->m_button_gradient_bot_focused   = flat_theme->m_button_gradient_top_focused;
    flat_theme->m_button_gradient_top_unfocused = flat_theme->m_transparent;
    flat_theme->m_button_gradient_bot_unfocused = flat_theme->m_transparent;
    flat_theme->m_button_gradient_top_pushed    = flat_theme->m_transparent;
    flat_theme->m_button_gradient_bot_pushed    = flat_theme->m_button_gradient_top_pushed;

    //
    // Construct the top-level widgets
    //

    m_top_panel = new Window(this, "");
    m_top_panel->set_theme(panel_theme);
    m_top_panel->set_position(nanogui::Vector2i(0, 0));
    m_top_panel->set_fixed_height(30);
    m_top_panel->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 5, 5));

    m_side_panel = new Window(this, "");
    m_side_panel->set_theme(panel_theme);

    m_tool_panel = new Window(this, "");
    m_tool_panel->set_theme(panel_theme);
    m_tool_panel->set_fixed_width(33);
    m_tool_panel->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 4, 4));

    m_image_view = new HDRImageView(this);
    m_image_view->set_grid_threshold(10);
    m_image_view->set_pixel_info_threshold(40);

    m_status_bar = new Window(this, "");
    m_status_bar->set_theme(panel_theme);
    m_status_bar->set_fixed_height(m_status_bar->theme()->m_text_box_font_size + 1);

    //
    // create status bar widgets
    //

    m_status_label = new Label(m_status_bar, "", "sans");
    m_status_label->set_font_size(theme->m_text_box_font_size);
    m_status_label->set_position(nanogui::Vector2i(6, 0));

    m_zoom_label = new Label(m_status_bar, "100% (1 : 1)", "sans");
    m_zoom_label->set_font_size(theme->m_text_box_font_size);

    //
    // create side panel widgets
    //

    {
        vector<Button *>    panel_btns;
        vector<Widget *>    panels;
        vector<PopupMenu *> popup_menus;
        m_side_scroll_panel   = new VScrollPanel(m_side_panel);
        m_side_panel_contents = new Widget(m_side_scroll_panel);
        m_side_panel_contents->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 4, 4));
        m_side_panel_contents->set_fixed_width(215);
        m_side_scroll_panel->set_fixed_width(m_side_panel_contents->fixed_width() + 12);
        m_side_panel->set_fixed_width(m_side_scroll_panel->fixed_width());

        //
        // create file/images panel
        //

        // auto menu1 = new PopupMenu(this, m_side_panel);
        // auto menu2 = new PopupMenu(this, m_side_panel);

        popup_menus.push_back(new PopupMenu(this, m_side_panel));
        panel_btns.push_back(
            m_side_panel_contents->add<PopupWrapper>(popup_menus.back())->add<Button>("File", FA_CARET_DOWN));
        panel_btns.back()->set_theme(flat_theme);
        panel_btns.back()->set_flags(Button::ToggleButton);
        panel_btns.back()->set_pushed(true);
        panel_btns.back()->set_font_size(18);
        panel_btns.back()->set_icon_position(Button::IconPosition::Left);

        m_images_panel = new ImageListPanel(m_side_panel_contents, this, m_image_view);
        panels.push_back(m_images_panel);

        //
        // create info panel
        //

        popup_menus.push_back(new PopupMenu(this, m_side_panel));
        panel_btns.push_back(
            m_side_panel_contents->add<PopupWrapper>(popup_menus.back())->add<Button>("Info", FA_CARET_RIGHT));
        panel_btns.back()->set_theme(flat_theme);
        panel_btns.back()->set_flags(Button::ToggleButton);
        panel_btns.back()->set_pushed(false);
        panel_btns.back()->set_font_size(18);
        panel_btns.back()->set_icon_position(Button::IconPosition::Left);

        {
            auto info_panel = new Well(m_side_panel_contents, 1, Color(150, 32), Color(0, 50));
            info_panel->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 10, 5));

            auto row = new Widget(info_panel);
            row->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 0, 5));

            new Label(row, "File:", "sans-bold");
            m_path_info_label = new Label(row, "");
            m_path_info_label->set_fixed_width(135);

            row = new Widget(info_panel);
            row->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 0, 5));

            new Label(row, "Resolution:", "sans-bold");
            m_res_info_label = new Label(row, "");
            m_res_info_label->set_fixed_width(135);

            // spacer
            (new Widget(info_panel))->set_fixed_height(5);

            row = new Widget(info_panel);
            row->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 0, 5));

            auto tb = new ToolButton(row, FA_EYE_DROPPER);
            tb->set_theme(flat_theme);
            tb->set_enabled(false);
            tb->set_icon_extra_scale(1.5f);

            (new Label(row, "R:\nG:\nB:\nA:", "sans-bold"))->set_fixed_width(15);
            m_color32_info_label = new Label(row, "");
            m_color32_info_label->set_fixed_width(50 + 24 + 5);
            m_color8_info_label = new Label(row, "");
            m_color8_info_label->set_fixed_width(50);

            // spacer
            (new Widget(info_panel))->set_fixed_height(5);

            row = new Widget(info_panel);
            row->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 0, 5));

            tb = new ToolButton(row, FA_CROSSHAIRS);
            tb->set_theme(flat_theme);
            tb->set_enabled(false);
            tb->set_icon_extra_scale(1.5f);

            (new Label(row, "X:\nY:", "sans-bold"))->set_fixed_width(15);
            m_pixel_info_label = new Label(row, "");
            m_pixel_info_label->set_fixed_width(50);

            tb = new ToolButton(row, FA_EXPAND);
            tb->set_theme(flat_theme);
            tb->set_enabled(false);
            tb->set_icon_extra_scale(1.5f);

            (new Label(row, "W:\nH:", "sans-bold"))->set_fixed_width(20);
            m_roi_info_label = new Label(row, "");
            m_roi_info_label->set_fixed_width(50);

            // spacer
            (new Widget(info_panel))->set_fixed_height(5);

            row = new Widget(info_panel);
            row->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 0, 5));

            tb = new ToolButton(row, FA_PERCENTAGE);
            tb->set_theme(flat_theme);
            tb->set_enabled(false);
            tb->set_icon_extra_scale(1.5f);

            (new Label(row, "Min:\nAvg:\nMax:", "sans-bold"))->set_fixed_width(30);
            m_stats_label = new Label(row, "");
            m_stats_label->set_fixed_width(135);

            info_panel->set_fixed_height(4);
            panels.push_back(info_panel);
        }

        //
        // create edit panel
        //

        popup_menus.push_back(new PopupMenu(this, m_side_panel));
        panel_btns.push_back(
            m_side_panel_contents->add<PopupWrapper>(popup_menus.back())->add<Button>("Edit", FA_CARET_RIGHT));
        panel_btns.back()->set_theme(flat_theme);
        panel_btns.back()->set_flags(Button::ToggleButton);
        panel_btns.back()->set_font_size(18);
        panel_btns.back()->set_icon_position(Button::IconPosition::Left);

        m_edit_panel = new EditImagePanel(m_side_panel_contents, this, m_images_panel, m_image_view);
        m_edit_panel->set_fixed_height(4);
        panels.push_back(m_edit_panel);

        //
        // panel callbacks
        //

        static bool solo = false;

        auto toggle_panel = [this, panel_btns, panels](size_t index, bool value)
        {
            // expand of collapse the specified panel
            panel_btns[index]->set_icon(value ? FA_CARET_DOWN : FA_CARET_RIGHT);
            panel_btns[index]->set_pushed(value);
            panels[index]->set_fixed_height(value ? 0 : 4);
            panels[index]->set_visible(true);

            // close other panels if solo mode is on
            if (value && solo)
            {
                for (size_t i = 0; i < panels.size(); ++i)
                {
                    if (i == index)
                        continue;
                    panel_btns[i]->set_pushed(false);
                    panel_btns[i]->set_icon(FA_CARET_RIGHT);
                    panels[i]->set_fixed_height(4);
                }
            }

            request_layout_update();
            m_side_panel_contents->perform_layout(m_nvg_context);
        };

        for (size_t i = 0; i < panel_btns.size(); ++i)
            panel_btns[i]->set_change_callback([i, toggle_panel](bool value) { toggle_panel(i, value); });

        //
        // create the right-click menus
        //
        for (size_t i = 0; i < popup_menus.size(); ++i)
        {
            popup_menus[i]->add_item("Solo mode");
            popup_menus[i]->add_item(""); // separator
            popup_menus[i]->add_item("Expand all");
            popup_menus[i]->add_item("Collapse all");
        }

        for (size_t i = 0; i < popup_menus.size(); ++i)
        {
            dynamic_cast<Button *>(popup_menus[i]->child_at(0))
                ->set_callback(
                    [popup_menus, panel_btns, toggle_panel, i]
                    {
                        solo = !solo;
                        // update all "Solo mode" buttons and "Expand all" buttons
                        for (size_t j = 0; j < popup_menus.size(); ++j)
                        {
                            dynamic_cast<Button *>(popup_menus[j]->child_at(0))->set_icon(solo ? FA_CHECK : 0);
                            popup_menus[j]->child_at(2)->set_enabled(!solo);
                        }

                        toggle_panel(i, solo ? true : panel_btns[i]->pushed());
                    });

            dynamic_cast<Button *>(popup_menus[i]->child_at(2))
                ->set_callback(
                    [panel_btns, toggle_panel, popup_menus, i]
                    {
                        // expand this panel, and all other panels if solo mode isn't on
                        for (size_t j = 0; j < popup_menus.size(); ++j)
                        {
                            if (j == i || !solo)
                                toggle_panel(j, true);
                        }
                    });

            dynamic_cast<Button *>(popup_menus[i]->child_at(3))
                ->set_callback(
                    [panel_btns, toggle_panel, popup_menus]
                    {
                        // collapse all panels
                        for (size_t j = 0; j < popup_menus.size(); ++j) toggle_panel(j, false);
                    });
        }
    }

    //
    // create top panel controls
    //

    m_help_button = new Button{m_top_panel, "", FA_QUESTION};
    m_help_button->set_icon_extra_scale(1.25f);
    m_help_button->set_fixed_size(nanogui::Vector2i(25, 25));
    m_help_button->set_change_callback([this](bool) { toggle_help_window(); });
    m_help_button->set_tooltip("Information about using HDRView.");
    m_help_button->set_flags(Button::ToggleButton);

    m_side_panel_button = new Button(m_top_panel, "", FA_BARS);
    m_side_panel_button->set_icon_extra_scale(1.25f);
    m_side_panel_button->set_tooltip(
        "Bring up the images dialog to load/remove images, and cycle through open images.");
    m_side_panel_button->set_flags(Button::ToggleButton);
    m_side_panel_button->set_pushed(true);
    m_side_panel_button->set_fixed_size(nanogui::Vector2i(25, 25));
    m_side_panel_button->set_change_callback(
        [this](bool value)
        {
            m_gui_animation_start = glfwGetTime();
            push_gui_refresh();
            m_animation_running = true;
            m_animation_goal    = EAnimationGoal(m_animation_goal ^ SIDE_PANEL);
            request_layout_update();
        });

    //
    // create tools
    //

    m_tools.push_back(new HandTool(this, m_image_view, m_images_panel));
    m_tools.push_back(new RectangularMarquee(this, m_image_view, m_images_panel));
    m_tools.push_back(new BrushTool(this, m_image_view, m_images_panel));
    m_tools.push_back(new EraserTool(this, m_image_view, m_images_panel));
    m_tools.push_back(new CloneStampTool(this, m_image_view, m_images_panel));
    m_tools.push_back(new Eyedropper(this, m_image_view, m_images_panel));

    for (auto t : m_tools) t->create_toolbutton(m_tool_panel);

    (new Widget(m_tool_panel))->set_fixed_height(15);

    m_color_btns = new DualHDRColorPicker(m_tool_panel);
    m_color_btns->set_fixed_size(Vector2i(25));
    // m_color_btns->set_tooltip("Select foreground and background colors.");
    m_color_btns->foreground()->popup()->set_anchor_offset(100);
    m_color_btns->background()->popup()->set_anchor_offset(100);
    m_color_btns->foreground()->set_side(Popup::Left);
    m_color_btns->background()->set_side(Popup::Left);

    m_color_btns->foreground()->set_tooltip("Set foreground color.");
    m_color_btns->background()->set_tooltip("Set background color.");
    m_color_btns->foreground()->set_eyedropper_callback(
        [this](bool pushed) { set_active_colorpicker(pushed ? m_color_btns->foreground() : nullptr); });
    m_color_btns->background()->set_eyedropper_callback(
        [this](bool pushed) { set_active_colorpicker(pushed ? m_color_btns->background() : nullptr); });

    {
        // create default options bar
        auto default_tool = m_tools[Tool::Tool_None]->create_options_bar(m_top_panel);

        // marquee tool uses the default options bar
        m_tools[Tool::Tool_Rectangular_Marquee]->set_options_bar(default_tool);

        // brush tool uses its own options bar
        m_tools[Tool::Tool_Brush]->create_options_bar(m_top_panel);

        // brush tool uses its own options bar
        m_tools[Tool::Tool_Eraser]->create_options_bar(m_top_panel);

        // clone stamp uses its own options bar
        m_tools[Tool::Tool_Clone_Stamp]->create_options_bar(m_top_panel);

        // eyedropper uses its own options bar
        m_tools[Tool::Tool_Eyedropper]->create_options_bar(m_top_panel);
    }

    m_tools[Tool::Tool_None]->set_active(true);

    m_image_view->set_zoom_callback(
        [this](float zoom)
        {
            float real_zoom = zoom * pixel_ratio();
            int   numer     = (real_zoom < 1.0f) ? 1 : (int)round(real_zoom);
            int   denom     = (real_zoom < 1.0f) ? (int)round(1.0f / real_zoom) : 1;
            m_zoom_label->set_caption(fmt::format("{:7.2f}% ({:d} : {:d})", real_zoom * 100, numer, denom));
            request_layout_update();
        });

    m_image_view->set_pixel_callback(
        [this](const nanogui::Vector2i &index, char **out, size_t size)
        {
            auto img = m_images_panel->current_image();
            if (img)
            {
                const HDRImage &image = img->image();
                for (int ch = 0; ch < 4; ++ch)
                {
                    float value = image(index.x(), index.y())[ch];
                    snprintf(out[ch], size, "%f", value);
                }
            }
        });

    m_image_view->set_mouse_callback([this](const Vector2i &p, int button, bool down, int modifiers) -> bool
                                     { return m_tools[m_tool]->mouse_button(p, button, down, modifiers); });

    m_image_view->set_drag_callback([this](const Vector2i &p, const Vector2i &rel, int button, int modifiers) -> bool
                                    { return m_tools[m_tool]->mouse_drag(p, rel, button, modifiers); });

    m_image_view->set_draw_callback([this](NVGcontext *ctx) { m_tools[m_tool]->draw(ctx); });

    m_image_view->set_hover_callback(
        [this](const Vector2i &pixel, const Color4 &color32, const Color4 &color8)
        {
            if (pixel.x() >= 0)
            {
                m_status_label->set_caption(fmt::format(
                    "({: 4d},{: 4d}) = ({: 6.3f}, {: 6.3f}, {: 6.3f}, {: 6.3f}) / ({: 3d}, {: 3d}, {: 3d}, {: 3d})",
                    pixel.x(), pixel.y(), color32[0], color32[1], color32[2], color32[3], (int)round(color8[0]),
                    (int)round(color8[1]), (int)round(color8[2]), (int)round(color8[3])));

                m_color32_info_label->set_caption(fmt::format("{: 6.3f}\n{: 6.3f}\n{: 6.3f}\n{: 6.3f}", color32[0],
                                                              color32[1], color32[2], color32[3]));

                m_color8_info_label->set_caption(fmt::format("{: 3d}\n{: 3d}\n{: 3d}\n{: 3d}", (int)round(color8[0]),
                                                             (int)round(color8[1]), (int)round(color8[2]),
                                                             (int)round(color8[3])));

                m_pixel_info_label->set_caption(fmt::format("{: 4d}\n{: 4d}", pixel.x(), pixel.y()));

                if (m_active_colorpicker)
                    m_active_colorpicker->set_color(Color(color32[0], color32[1], color32[2], color32[3]));
            }
            else
            {
                m_status_label->set_caption("");
                m_color32_info_label->set_caption("");
                m_color8_info_label->set_caption("");
                m_pixel_info_label->set_caption("");
            }

            m_status_bar->perform_layout(m_nvg_context);
        });

    m_image_view->set_changed_callback(
        [this]()
        {
            if (auto img = m_images_panel->current_image())
            {
                m_path_info_label->set_caption(fmt::format("{}", img->filename()));
                m_res_info_label->set_caption(fmt::format("{} × {}", img->width(), img->height()));

                perform_layout();
            }
            else
            {
                m_path_info_label->set_caption("");
                m_res_info_label->set_caption("");
                m_stats_label->set_caption("");
            }
        });

    drop_event(args);

    this->set_size(nanogui::Vector2i(1024, 800));
    request_layout_update();
    set_resize_callback([&](nanogui::Vector2i) { request_layout_update(); });

    set_visible(true);
    glfwSwapInterval(1);

    // Nanogui will redraw the screen for key/mouse events, but we need to manually
    // invoke redraw for things like gui animations. do this in a separate thread
    m_gui_refresh_thread = std::thread(
        [this]()
        {
            std::chrono::microseconds idle_quantum = std::chrono::microseconds(1000 * 1000 / 20);
            std::chrono::microseconds anim_quantum = std::chrono::microseconds(1000 * 1000 / 120);
            while (true)
            {
                bool anim = this->should_refresh_gui();
                std::this_thread::sleep_for(anim ? anim_quantum : idle_quantum);
                this->redraw();
                if (anim)
                    spdlog::trace("refreshing gui");
            }
        });
}

HDRViewScreen::~HDRViewScreen() { m_gui_refresh_thread.join(); }

void HDRViewScreen::set_active_colorpicker(HDRColorPicker *cp)
{
    spdlog::trace("setting colorpicker to {}", intptr_t(cp));
    if (m_images_panel->current_image())
    {
        m_active_colorpicker = cp;
        if (cp)
            push_gui_refresh();
        else
            pop_gui_refresh();

        set_tool(Tool::Tool_Eyedropper);
    }
    else
    {
        m_active_colorpicker = nullptr;
        set_tool(Tool::Tool_None);
    }
}

void HDRViewScreen::set_tool(Tool::ETool t)
{
    m_tool = t;
    for (int i = 0; i < (int)Tool::Tool_Num_Tools; ++i) m_tools[i]->set_active(false);

    m_tools[t]->set_active(true);
}

void HDRViewScreen::update_caption()
{
    auto img = m_images_panel->current_image();
    if (img)
        set_caption(string("HDRView [") + img->filename() + (img->is_modified() ? "*" : "") + "]");
    else
        set_caption(string("HDRView"));
}

void HDRViewScreen::bring_to_focus() const { glfwFocusWindow(m_glfw_window); }

bool HDRViewScreen::drop_event(const vector<string> &filenames)
{
    try
    {
        m_images_panel->load_images(filenames);

        bring_to_focus();

        // Ensure the new image button will have the correct visibility state.
        m_images_panel->set_filter(m_images_panel->filter());

        request_layout_update();
    }
    catch (const exception &e)
    {
        new MessageDialog(this, MessageDialog::Type::Warning, "Error", string("Could not load:\n ") + e.what());
        return false;
    }
    return true;
}

void HDRViewScreen::ask_close_image(int)
{
    auto closeit = [this](int curr, int next)
    {
        m_images_panel->close_image();
        cout << "curr: " << m_images_panel->current_image_index() << endl;
    };

    auto curr = m_images_panel->current_image_index();
    auto next = m_images_panel->next_visible_image(curr, Forward);
    cout << "curr: " << curr << "; next: " << next << endl;
    if (auto img = m_images_panel->image(curr))
    {
        if (img->is_modified())
        {
            auto dialog = new MessageDialog(this, MessageDialog::Type::Warning, "Warning!",
                                            "Image has unsaved modifications. Close anyway?", "Yes", "Cancel", true);
            dialog->set_callback(
                [curr, next, closeit](int close)
                {
                    if (close == 0)
                        closeit(curr, next);
                });
        }
        else
            closeit(curr, next);
    }
}

void HDRViewScreen::ask_close_all_images()
{
    bool anyModified = false;
    for (int i = 0; i < m_images_panel->num_images(); ++i) anyModified |= m_images_panel->image(i)->is_modified();

    if (anyModified)
    {
        auto dialog = new MessageDialog(this, MessageDialog::Type::Warning, "Warning!",
                                        "Some images have unsaved modifications. Close all images anyway?", "Yes",
                                        "Cancel", true);
        dialog->set_callback(
            [this](int close)
            {
                if (close == 0)
                    m_images_panel->close_all_images();
            });
    }
    else
        m_images_panel->close_all_images();
}

void HDRViewScreen::toggle_help_window()
{
    if (m_help_window)
    {
        m_help_window->dispose();
        m_help_window = nullptr;
        m_help_button->set_pushed(false);
    }
    else
    {
        m_help_window = new HelpWindow{this, [this] { toggle_help_window(); }};
        m_help_window->center();
        m_help_window->request_focus();
        m_help_button->set_pushed(true);
    }

    request_layout_update();
}

bool HDRViewScreen::load_image()
{
    vector<string> files = file_dialog({{"exr", "OpenEXR image"},
                                        {"dng", "Digital Negative raw image"},
                                        {"png", "Portable Network Graphic image"},
                                        {"pfm", "Portable FloatMap image"},
                                        {"ppm", "Portable PixMap image"},
                                        {"pnm", "Portable AnyMap image"},
                                        {"jpg", "JPEG image"},
                                        {"tga", "Truevision Targa image"},
                                        {"pic", "Softimage PIC image"},
                                        {"bmp", "Windows Bitmap image"},
                                        {"gif", "Graphics Interchange Format image"},
                                        {"hdr", "Radiance rgbE format image"},
                                        {"psd", "Photoshop document"}},
                                       false, true);

    // re-gain focus
    glfwFocusWindow(m_glfw_window);

    if (!files.empty())
        return drop_event(files);
    return false;
}

void HDRViewScreen::new_image()
{
    static int    width = 800, height = 600;
    static string name = "New image...";
    static Color  bg(0, 255);
    static float  EV = 0.f;

    FormHelper *gui = new FormHelper(this);
    gui->set_fixed_size(Vector2i(0, 20));

    auto window = gui->add_window(Vector2i(10, 10), name);
    window->set_modal(true);

    if (m_images_panel->current_image() && m_images_panel->current_image()->roi().has_volume())
    {
        width  = m_images_panel->current_image()->roi().size().x();
        height = m_images_panel->current_image()->roi().size().y();
    }

    {
        auto w = gui->add_variable("Width:", width);
        w->set_spinnable(true);
        w->set_min_value(1);
        w->set_units("px");
    }

    {
        auto h = gui->add_variable("Height:", height);
        h->set_spinnable(true);
        h->set_min_value(1);
        h->set_units("px");
    }

    auto spacer = new Widget(window);
    spacer->set_fixed_height(5);
    gui->add_widget("", spacer);

    bg             = background()->color();
    EV             = background()->exposure();
    auto color_btn = new HDRColorPicker(window, bg, EV);
    color_btn->popup()->set_anchor_offset(color_btn->popup()->height());
    color_btn->set_eyedropper_callback([this, color_btn](bool pushed)
                                       { set_active_colorpicker(pushed ? color_btn : nullptr); });
    gui->add_widget("Background color:", color_btn);
    color_btn->set_final_callback(
        [](const Color &c, float e)
        {
            bg = c;
            EV = e;
        });

    auto popup = color_btn->popup();
    request_layout_update();

    spacer = new Widget(window);
    spacer->set_fixed_height(15);
    gui->add_widget("", spacer);

    auto row = new Widget(window);
    row->set_layout(new GridLayout(Orientation::Horizontal, 2, Alignment::Fill, 0, 5));

    auto b = new Button(row, "Cancel", window->theme()->m_message_alt_button_icon);
    b->set_callback(
        [window, popup]()
        {
            window->dispose();
            popup->dispose();
        });
    b = new Button(row, "OK", window->theme()->m_message_primary_button_icon);
    b->set_callback(
        [this, window, popup]()
        {
            float gain = powf(2.f, EV);

            HDRImagePtr img = make_shared<HDRImage>(width, height, Color4(bg[0], bg[1], bg[2], bg[3]) * gain);
            m_images_panel->new_image(img);

            bring_to_focus();

            // Ensure the new image button will have the correct visibility state.
            m_images_panel->set_filter(m_images_panel->filter());

            request_layout_update();

            window->dispose();
            popup->dispose();
        });
    gui->add_widget("", row);

    window->center();
    window->request_focus();
}

void HDRViewScreen::duplicate_image()
{
    HDRImagePtr clipboard;
    if (auto img = m_images_panel->current_image())
    {
        auto roi = img->roi();
        if (!roi.has_volume())
            roi = img->box();
        clipboard = make_shared<HDRImage>(roi.size().x(), roi.size().y());
        clipboard->copy_subimage(img->image(), roi, 0, 0);
    }
    else
        clipboard = make_shared<HDRImage>(m_images_panel->current_image()->image());

    m_images_panel->new_image(clipboard);

    bring_to_focus();

    // Ensure the new image button will have the correct visibility state.
    m_images_panel->set_filter(m_images_panel->filter());

    request_layout_update();
}

void HDRViewScreen::save_image()
{
    try
    {
        if (!m_images_panel->current_image())
            return;

        string filename = file_dialog(
            {
                {"exr", "OpenEXR image"},
                {"hdr", "Radiance rgbE format image"},
                {"png", "Portable Network Graphic image"},
                {"pfm", "Portable FloatMap image"},
                {"ppm", "Portable PixMap image"},
                {"pnm", "Portable AnyMap image"},
                {"jpg", "JPEG image"},
                {"jpeg", "JPEG image"},
                {"tga", "Truevision Targa image"},
                {"bmp", "Windows Bitmap image"},
            },
            true);

        // re-gain focus
        glfwFocusWindow(m_glfw_window);

        if (!filename.empty())
            m_images_panel->save_image(filename, m_image_view->exposure(), m_image_view->gamma(), m_image_view->sRGB(),
                                       m_image_view->dithering_on());
    }
    catch (const exception &e)
    {
        new MessageDialog(this, MessageDialog::Type::Warning, "Error",
                          string("Could not save image due to an error:\n") + e.what());
    }
}

bool HDRViewScreen::keyboard_event(int key, int scancode, int action, int modifiers)
{
    if (Screen::keyboard_event(key, scancode, action, modifiers))
        return true;

    if (action == GLFW_RELEASE)
        return false;

    if (m_tools[m_tool]->keyboard(key, scancode, action, modifiers))
        return true;

    if (m_image_view->keyboard_event(key, scancode, action, modifiers))
        return true;

    if (m_images_panel->keyboard_event(key, scancode, action, modifiers))
        return true;

    switch (key)
    {
    case GLFW_KEY_ESCAPE:
    {
        if (!m_ok_to_quit_dialog)
        {
            m_ok_to_quit_dialog = new MessageDialog(this, MessageDialog::Type::Warning, "Warning!",
                                                    "Do you really want to quit?", "Yes", "No", true);
            m_ok_to_quit_dialog->set_callback(
                [this](int result)
                {
                    this->set_visible(result != 0);
                    m_ok_to_quit_dialog = nullptr;
                });
            m_ok_to_quit_dialog->request_focus();
        }
        else if (m_ok_to_quit_dialog->visible())
        {
            // dialog already visible, dismiss it
            m_ok_to_quit_dialog->dispose();
            m_ok_to_quit_dialog = nullptr;
        }
        return true;
    }

    case GLFW_KEY_ENTER:
    {
        if (m_ok_to_quit_dialog && m_ok_to_quit_dialog->visible())
            // quit dialog already visible, "enter" clicks OK, and quits
            m_ok_to_quit_dialog->callback()(0);
        else
            return true;
    }

    case GLFW_KEY_BACKSPACE:
        spdlog::trace("KEY BACKSPACE pressed");
        ask_close_image(m_images_panel->current_image_index());
        return true;

    case 'W':
        spdlog::trace("KEY `W` pressed");
        if (modifiers & SYSTEM_COMMAND_MOD)
        {
            if (modifiers & GLFW_MOD_SHIFT)
                ask_close_all_images();
            else
                ask_close_image(m_images_panel->current_image_index());
            return true;
        }
        return false;

    case 'O':
        spdlog::trace("KEY `O` pressed");
        if (modifiers & SYSTEM_COMMAND_MOD)
        {
            load_image();
            return true;
        }
        return false;

    case 'C':
        spdlog::trace("Key `C` pressed");
        if (modifiers & SYSTEM_COMMAND_MOD)
        {
            m_edit_panel->copy();
            return true;
        }
        break;

    case 'V':
        spdlog::trace("Key `V` pressed");
        if (modifiers & SYSTEM_COMMAND_MOD)
        {
            m_edit_panel->paste();
            return true;
        }
        break;

    case 'D':
        spdlog::trace("Key `D` pressed");
        m_color_btns->foreground()->set_color(Color(0, 255));
        m_color_btns->background()->set_color(Color(255, 255));
        return true;

    case 'X':
        spdlog::trace("Key `X` pressed");
        m_color_btns->swap_colors();
        return true;

    case 'T':
        spdlog::trace("KEY `T` pressed");
        m_gui_animation_start = glfwGetTime();
        push_gui_refresh();
        m_animation_running = true;
        m_animation_goal    = EAnimationGoal(m_animation_goal ^ TOP_PANEL);
        request_layout_update();
        return true;

    case 'H':
        spdlog::trace("KEY `H` pressed");
        toggle_help_window();
        return true;

    case GLFW_KEY_TAB:
        spdlog::trace("KEY TAB pressed");
        if (modifiers & GLFW_MOD_SHIFT)
        {
            bool setVis           = !((m_animation_goal & SIDE_PANEL) || (m_animation_goal & TOP_PANEL) ||
                            (m_animation_goal & BOTTOM_PANEL));
            m_gui_animation_start = glfwGetTime();
            push_gui_refresh();
            m_animation_running = true;
            m_animation_goal    = setVis ? EAnimationGoal(TOP_PANEL | SIDE_PANEL | BOTTOM_PANEL) : EAnimationGoal(0);
        }
        else
        {
            m_gui_animation_start = glfwGetTime();
            push_gui_refresh();
            m_animation_running = true;
            m_animation_goal    = EAnimationGoal(m_animation_goal ^ SIDE_PANEL);
        }

        request_layout_update();
        return true;
    }

    return false;
}

bool HDRViewScreen::at_side_panel_edge(const Vector2i &p)
{
    if (!(m_animation_goal & SIDE_PANEL))
        return false;
    auto w = find_widget(p);
    auto x = p.x() - m_side_panel->position().x() - m_side_panel->fixed_width();
    return x < 10 && x > -5 &&
           (w == m_side_panel || w == m_image_view || w == m_side_panel_contents || w == m_side_scroll_panel);
}

bool HDRViewScreen::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers)
{
    // temporarily increase the gui refresh rate between mouse down and up events.
    // makes things like dragging smoother
    if (down)
        push_gui_refresh();
    else
        pop_gui_refresh();

    if (m_active_colorpicker && down)
    {
        spdlog::trace("ending eyedropper");
        m_active_colorpicker->end_eyedropper();
        set_tool(Tool::Tool_None);
        return true;
    }

    // close all popup menus
    if (down)
    {
        bool closed_a_menu = false;
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
        {
            Widget *child = *it;
            if (child->visible() && !child->contains(p - m_pos) && dynamic_cast<PopupMenu *>(child))
            {
                child->set_visible(false);
                closed_a_menu = true;
            }
        }
        if (closed_a_menu)
            return true;
    }

    if (button == GLFW_MOUSE_BUTTON_1 && down && at_side_panel_edge(p))
    {
        m_dragging_side_panel = true;

        // prevent Screen::cursorPosCallbackEvent from calling drag_event on other widgets
        m_drag_active = false;
        m_drag_widget = nullptr;
        return true;
    }
    else
        m_dragging_side_panel = false;

    bool ret = Screen::mouse_button_event(p, button, down, modifiers);

    return ret;
}

bool HDRViewScreen::mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button,
                                       int modifiers)
{
    if ((m_dragging_side_panel || at_side_panel_edge(p)) && !m_active_colorpicker)
    {
        m_side_panel->set_cursor(Cursor::HResize);
        m_side_scroll_panel->set_cursor(Cursor::HResize);
        m_side_panel_contents->set_cursor(Cursor::HResize);
        m_image_view->set_cursor(Cursor::HResize);
    }
    else
    {
        m_side_panel->set_cursor(Cursor::Arrow);
        m_side_scroll_panel->set_cursor(Cursor::Arrow);
        m_side_panel_contents->set_cursor(Cursor::Arrow);
        m_image_view->set_cursor(Cursor::Arrow);
    }

    if (m_dragging_side_panel)
    {
        int w = ::clamp(p.x(), 215, m_size.x() - 10);
        m_side_panel_contents->set_fixed_width(w);
        m_side_scroll_panel->set_fixed_width(w + 12);
        m_side_panel->set_fixed_width(m_side_scroll_panel->fixed_width());
        request_layout_update();
        return true;
    }

    return Screen::mouse_motion_event(p, rel, button, modifiers);
}

void HDRViewScreen::update_layout()
{
    int header_height   = m_top_panel->fixed_height();
    int sidepanel_width = m_side_panel->fixed_width();
    int toolpanel_width = m_tool_panel->fixed_width();
    int footer_height   = m_status_bar->fixed_height();

    static int header_shift    = 0;
    static int sidepanel_shift = 0;
    static int toolpanel_shift = 0;
    static int footer_shift    = 0;

    if (m_animation_running)
    {
        const double duration = 0.2;
        double       elapsed  = glfwGetTime() - m_gui_animation_start;
        // stop the animation after 2 seconds
        if (elapsed > duration)
        {
            pop_gui_refresh();
            m_animation_running = false;
            sidepanel_shift     = (m_animation_goal & SIDE_PANEL) ? 0 : -sidepanel_width;
            toolpanel_shift     = (m_animation_goal & SIDE_PANEL) ? 0 : toolpanel_width;
            header_shift        = (m_animation_goal & TOP_PANEL) ? 0 : -header_height;
            footer_shift        = (m_animation_goal & BOTTOM_PANEL) ? 0 : footer_height;

            m_side_panel_button->set_pushed(m_animation_goal & SIDE_PANEL);
        }
        // animate the location of the panels
        else
        {
            // only animate the sidepanel if it isn't already at the goal position
            if (((m_animation_goal & SIDE_PANEL) && sidepanel_shift != 0) ||
                (!(m_animation_goal & SIDE_PANEL) && sidepanel_shift != -sidepanel_width))
            {
                double start    = (m_animation_goal & SIDE_PANEL) ? double(-sidepanel_width) : 0.0;
                double end      = (m_animation_goal & SIDE_PANEL) ? 0.0 : double(-sidepanel_width);
                sidepanel_shift = static_cast<int>(round(lerp(start, end, smoothStep(0.0, duration, elapsed))));

                start           = (m_animation_goal & SIDE_PANEL) ? double(toolpanel_width) : 0.0;
                end             = (m_animation_goal & SIDE_PANEL) ? 0.0 : double(toolpanel_width);
                toolpanel_shift = static_cast<int>(round(lerp(start, end, smoothStep(0.0, duration, elapsed))));
                m_side_panel_button->set_pushed(true);
            }
            // only animate the header if it isn't already at the goal position
            if (((m_animation_goal & TOP_PANEL) && header_shift != 0) ||
                (!(m_animation_goal & TOP_PANEL) && header_shift != -header_height))
            {
                double start = (m_animation_goal & TOP_PANEL) ? double(-header_height) : 0.0;
                double end   = (m_animation_goal & TOP_PANEL) ? 0.0 : double(-header_height);
                header_shift = static_cast<int>(round(lerp(start, end, smoothStep(0.0, duration, elapsed))));
            }

            // only animate the footer if it isn't already at the goal position
            if (((m_animation_goal & BOTTOM_PANEL) && footer_shift != 0) ||
                (!(m_animation_goal & BOTTOM_PANEL) && footer_shift != footer_height))
            {
                double start = (m_animation_goal & BOTTOM_PANEL) ? double(footer_height) : 0.0;
                double end   = (m_animation_goal & BOTTOM_PANEL) ? 0.0 : double(footer_height);
                footer_shift = static_cast<int>(round(lerp(start, end, smoothStep(0.0, duration, elapsed))));
            }
        }
    }

    m_top_panel->set_position(nanogui::Vector2i(0, header_shift));
    m_top_panel->set_fixed_width(width());

    int middle_height = height() - header_height - footer_height - header_shift + footer_shift;
    int middleWidth   = width() - toolpanel_width + toolpanel_shift;

    m_side_panel->set_position(nanogui::Vector2i(sidepanel_shift, header_shift + header_height));
    m_side_panel->set_fixed_height(middle_height);

    m_tool_panel->set_position(nanogui::Vector2i(middleWidth, header_shift + header_height));
    m_tool_panel->set_fixed_height(middle_height);

    m_image_view->set_position(nanogui::Vector2i(sidepanel_shift + sidepanel_width, header_shift + header_height));
    m_image_view->set_fixed_width(width() - sidepanel_shift - sidepanel_width - toolpanel_width + toolpanel_shift);
    m_image_view->set_fixed_height(middle_height);

    m_status_bar->set_position(nanogui::Vector2i(0, header_shift + header_height + middle_height));
    m_status_bar->set_fixed_width(width());

    int lh = std::min(middle_height, m_side_panel_contents->preferred_size(m_nvg_context).y());
    m_side_scroll_panel->set_fixed_height(lh);

    int zoomWidth = m_zoom_label->preferred_size(m_nvg_context).x();
    m_zoom_label->set_width(zoomWidth);
    m_zoom_label->set_position(nanogui::Vector2i(width() - zoomWidth - 6, 0));

    perform_layout();

    if (!m_dragging_side_panel)
    {
        // With a changed layout the relative position of the mouse
        // within children changes and therefore should get updated.
        // nanogui does not handle this for us.
        double x, y;
        glfwGetCursorPos(m_glfw_window, &x, &y);
        cursor_pos_callback_event(x, y);
    }
}

void HDRViewScreen::draw_all()
{
    if (m_redraw)
    {
        m_redraw = false;

        draw_setup();
        draw_contents();
        draw_widgets();
        draw_teardown();
    }
}

void HDRViewScreen::draw_contents()
{
    clear();

    m_images_panel->run_requested_callbacks();

    if (auto img = m_images_panel->current_image())
    {
        img->check_async_result();
        img->upload_to_GPU();

        if (!img->is_null() && img->histograms() && img->histograms()->ready() && img->histograms()->get())
        {
            auto lazyHist = img->histograms();
            m_stats_label->set_caption(fmt::format("{:.3f}\n{:.3f}\n{:.3f}", lazyHist->get()->minimum,
                                                   lazyHist->get()->average, lazyHist->get()->maximum));
        }
        else
            m_stats_label->set_caption("");

        m_roi_info_label->set_caption(
            img->roi().has_volume() ? fmt::format("{: 4d}\n{: 4d}", img->roi().size().x(), img->roi().size().y()) : "");
    }

    if (m_need_layout_update || m_animation_running)
    {
        update_layout();
        // redraw();
        m_need_layout_update = false;
    }
}

void HDRViewScreen::draw_widgets()
{
    nvgBeginFrame(m_nvg_context, m_size[0], m_size[1], m_pixel_ratio);

    draw(m_nvg_context);

    // copied from nanogui::Screen
    // FIXME: prevent tooltips from running off right edge of screen.
    double elapsed = glfwGetTime() - m_last_interaction;
    if (elapsed > 0.5f)
    {
        // Draw tooltips
        const Widget *widget = find_widget(m_mouse_pos);
        if (widget && !widget->tooltip().empty())
        {
            int tooltip_width = 150;

            float bounds[4];
            nvgFontFace(m_nvg_context, "sans");
            nvgFontSize(m_nvg_context, 15.0f);
            nvgTextAlign(m_nvg_context, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            nvgTextLineHeight(m_nvg_context, 1.1f);
            Vector2i pos = widget->absolute_position() + Vector2i(widget->width() / 2, widget->height() + 10);

            nvgTextBounds(m_nvg_context, pos.x(), pos.y(), widget->tooltip().c_str(), nullptr, bounds);

            int w = (bounds[2] - bounds[0]) / 2;
            if (w > tooltip_width / 2)
            {
                nvgTextAlign(m_nvg_context, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                nvgTextBoxBounds(m_nvg_context, pos.x(), pos.y(), tooltip_width, widget->tooltip().c_str(), nullptr,
                                 bounds);

                w = (bounds[2] - bounds[0]) / 2;
            }
            int shift = 0;

            if (pos.x() - w - 8 < 0)
            {
                // Keep tooltips on screen
                shift = pos.x() - w - 8;
                pos.x() -= shift;
                bounds[0] -= shift;
                bounds[2] -= shift;
            }
            else if (pos.x() + w + 8 > width())
            {
                // Keep tooltips on screen
                shift = pos.x() + w + 8 - width();
                pos.x() -= shift;
                bounds[0] -= shift;
                bounds[2] -= shift;
            }

            nvgGlobalAlpha(m_nvg_context, std::min(1.0, 2 * (elapsed - 0.5f)) * 0.8);

            nvgBeginPath(m_nvg_context);
            nvgFillColor(m_nvg_context, Color(0, 255));
            nvgRoundedRect(m_nvg_context, bounds[0] - 4 - w, bounds[1] - 4, (int)(bounds[2] - bounds[0]) + 8,
                           (int)(bounds[3] - bounds[1]) + 8, 3);

            int px = (int)((bounds[2] + bounds[0]) / 2) - w + shift;
            nvgMoveTo(m_nvg_context, px, bounds[1] - 10);
            nvgLineTo(m_nvg_context, px + 7, bounds[1] + 1);
            nvgLineTo(m_nvg_context, px - 7, bounds[1] + 1);
            nvgFill(m_nvg_context);

            nvgFillColor(m_nvg_context, Color(255, 255));
            nvgFontBlur(m_nvg_context, 0.0f);
            nvgTextBox(m_nvg_context, pos.x() - w, pos.y(), tooltip_width, widget->tooltip().c_str(), nullptr);
        }
    }

    nvgEndFrame(m_nvg_context);
}
