use crate::tech_tree::{TechTree, TechBox, TechStatus, Age};
use raylib::prelude::*;

pub struct TechTreeUI {
    pub scroll_offset: Vector2,
    pub zoom: f32,
    pub hovered_tech: Option<crate::tech_tree::TechId>,
}

impl TechTreeUI {
    pub fn new() -> Self {
        Self {
            scroll_offset: Vector2::zero(),
            zoom: 1.0,
            hovered_tech: None,
        }
    }

    pub fn draw_full_tree(&mut self, d: &mut RaylibDrawHandle, tech_tree: &TechTree) {
        if !tech_tree.is_open {
            return;
        }

        let screen_width = d.get_screen_width();
        let screen_height = d.get_screen_height();

        // Draw background overlay
        d.draw_rectangle(0, 0, screen_width, screen_height, Color::new(0, 0, 0, 200));

        // Title
        d.draw_text("TECHNOLOGY TREE", 50, 30, 32, Color::WHITE);
        d.draw_text("Press T to close", screen_width - 200, 30, 16, Color::LIGHTGRAY);

        // Draw age sections
        let mut current_y = 100;
        let ages = [Age::StoneAge, Age::EarlyCivilization, Age::IronAge, Age::IndustrialAge];
        
        for &age in &ages {
            current_y = self.draw_age_section(d, tech_tree, age, current_y);
            current_y += 50; // Gap between ages
        }

        // Connections removed for cleaner UI

        // Draw hover tooltip
        if let Some(hovered_id) = self.hovered_tech {
            if let Some(tech) = tech_tree.techs.get(&hovered_id) {
                self.draw_tech_tooltip(d, tech, d.get_mouse_position());
            }
        }
    }

    fn draw_age_section(&mut self, d: &mut RaylibDrawHandle, tech_tree: &TechTree, age: Age, start_y: i32) -> i32 {
        let screen_width = d.get_screen_width();
        
        // Age header
        let age_color = age.get_color();
        let age_name = age.get_name();
        
        // Check if age is unlocked
        let age_unlocked = age as u32 <= tech_tree.current_age as u32;
        let header_color = if age_unlocked { age_color } else { Color::GRAY };
        
        d.draw_text(age_name, 50, start_y, 24, header_color);
        
        // Age requirements (if locked)
        if !age_unlocked {
            let required_techs = age.get_required_techs();
            let completed_count = required_techs.iter()
                .filter(|tech_id| tech_tree.completed_techs.contains(tech_id))
                .count();
            
            let requirement_text = format!("Requires: {}/{} technologies", completed_count, required_techs.len());
            d.draw_text(&requirement_text, 50, start_y + 30, 14, Color::LIGHTGRAY);
        }

        // Draw tech boxes for this age
        let techs = tech_tree.get_techs_for_age(age);
        let mut max_y = start_y + 60;
        
        for (i, tech) in techs.iter().enumerate() {
            let box_x = 100 + (i % 4) * 200; // 4 techs per row
            let box_y = start_y + 60 + ((i / 4) * 120) as i32;
            
            self.draw_tech_box(d, tech, box_x as i32, box_y);
            max_y = max_y.max(box_y + 100);
        }

        max_y
    }

    fn draw_tech_box(&mut self, d: &mut RaylibDrawHandle, tech: &TechBox, x: i32, y: i32) {
        let box_width = 180;
        let box_height = 100;
        
        // Check if mouse is hovering
        let mouse_pos = d.get_mouse_position();
        let hovering = mouse_pos.x >= x as f32 && mouse_pos.x <= (x + box_width) as f32 &&
                      mouse_pos.y >= y as f32 && mouse_pos.y <= (y + box_height) as f32;
        
        if hovering {
            self.hovered_tech = Some(tech.id);
        }

        // Box background
        let bg_color = if hovering {
            Color::new(60, 60, 60, 255)
        } else {
            Color::new(40, 40, 40, 255)
        };
        d.draw_rectangle(x, y, box_width, box_height, bg_color);

        // Status border
        let border_color = tech.get_status_color();
        let border_width = if hovering { 3 } else { 2 };
        d.draw_rectangle_lines_ex(
            Rectangle::new(x as f32, y as f32, box_width as f32, box_height as f32),
            border_width as f32,
            border_color
        );

        // Tech name
        let name_color = match tech.status {
            TechStatus::Locked => Color::GRAY,
            _ => Color::WHITE,
        };
        d.draw_text(tech.name, x + 10, y + 10, 16, name_color);

        // Progress bar
        if tech.status != TechStatus::Locked {
            let progress_y = y + 35;
            let progress_width = box_width - 20;
            let progress_height = 8;
            
            // Background
            d.draw_rectangle(x + 10, progress_y, progress_width, progress_height, Color::DARKGRAY);
            
            // Fill
            let completion_ratio = tech.get_completion_ratio();
            let fill_width = (progress_width as f32 * completion_ratio) as i32;
            let fill_color = match tech.status {
                TechStatus::Completed => Color::GREEN,
                TechStatus::InProgress => Color::YELLOW,
                _ => Color::BLUE,
            };
            d.draw_rectangle(x + 10, progress_y, fill_width, progress_height, fill_color);
            
            // Progress text
            let completed = tech.objectives.iter().filter(|obj| obj.completed).count();
            let total = tech.objectives.len();
            let progress_text = format!("{}/{}", completed, total);
            d.draw_text(&progress_text, x + 10, progress_y + 12, 12, Color::LIGHTGRAY);
        }

        // Status icon
        let status_text = match tech.status {
            TechStatus::Locked => "🔒",
            TechStatus::Available => "🔓",
            TechStatus::InProgress => "⚠",
            TechStatus::Completed => "✓",
        };
        d.draw_text(status_text, x + box_width - 30, y + 10, 20, tech.get_status_color());

        // Short description
        let desc_lines = self.wrap_text(tech.description, 20);
        for (i, line) in desc_lines.iter().take(2).enumerate() {
            d.draw_text(line, x + 10, y + 55 + i as i32 * 15, 11, Color::LIGHTGRAY);
        }
    }


    fn draw_tech_tooltip(&self, d: &mut RaylibDrawHandle, tech: &TechBox, mouse_pos: Vector2) {
        let tooltip_width = 300;
        let tooltip_height = 150 + tech.objectives.len() as i32 * 20;
        
        let mut tooltip_x = mouse_pos.x as i32 + 10;
        let mut tooltip_y = mouse_pos.y as i32 + 10;
        
        // Keep tooltip on screen
        if tooltip_x + tooltip_width > d.get_screen_width() {
            tooltip_x = mouse_pos.x as i32 - tooltip_width - 10;
        }
        if tooltip_y + tooltip_height > d.get_screen_height() {
            tooltip_y = mouse_pos.y as i32 - tooltip_height - 10;
        }

        // Background
        d.draw_rectangle(tooltip_x, tooltip_y, tooltip_width, tooltip_height, Color::new(20, 20, 20, 240));
        d.draw_rectangle_lines(tooltip_x, tooltip_y, tooltip_width, tooltip_height, Color::WHITE);

        let mut current_y = tooltip_y + 10;

        // Title
        d.draw_text(tech.name, tooltip_x + 10, current_y, 18, Color::WHITE);
        current_y += 25;

        // Description
        d.draw_text(tech.description, tooltip_x + 10, current_y, 12, Color::LIGHTGRAY);
        current_y += 20;

        // Prerequisites
        if !tech.prerequisites.is_empty() {
            d.draw_text("Prerequisites:", tooltip_x + 10, current_y, 14, Color::YELLOW);
            current_y += 20;
            for prereq_id in &tech.prerequisites {
                let prereq_name = format!("• {:?}", prereq_id); // In real implementation, get actual name
                d.draw_text(&prereq_name, tooltip_x + 20, current_y, 12, Color::LIGHTGRAY);
                current_y += 18;
            }
        }

        // Objectives
        d.draw_text("Objectives:", tooltip_x + 10, current_y, 14, Color::YELLOW);
        current_y += 20;
        
        for objective in &tech.objectives {
            let obj_color = if objective.completed { Color::GREEN } else { Color::WHITE };
            let obj_text = objective.get_progress_text();
            d.draw_text(&obj_text, tooltip_x + 20, current_y, 12, obj_color);
            current_y += 18;
        }

        // Unlocks
        if !tech.unlocked_recipes.is_empty() || !tech.unlocked_buildings.is_empty() {
            current_y += 10;
            d.draw_text("Unlocks:", tooltip_x + 10, current_y, 14, Color::YELLOW);
            current_y += 20;
            
            for recipe in &tech.unlocked_recipes {
                let unlock_text = format!("• {:?}", recipe); // In real implementation, get actual name
                d.draw_text(&unlock_text, tooltip_x + 20, current_y, 12, Color::LIGHTGRAY);
                current_y += 18;
            }
        }
    }

    fn wrap_text(&self, text: &str, max_chars: usize) -> Vec<String> {
        let words: Vec<&str> = text.split_whitespace().collect();
        let mut lines = Vec::new();
        let mut current_line = String::new();

        for word in words {
            if current_line.len() + word.len() + 1 <= max_chars {
                if !current_line.is_empty() {
                    current_line.push(' ');
                }
                current_line.push_str(word);
            } else {
                if !current_line.is_empty() {
                    lines.push(current_line);
                    current_line = String::new();
                }
                current_line.push_str(word);
            }
        }

        if !current_line.is_empty() {
            lines.push(current_line);
        }

        lines
    }

    pub fn draw_current_objectives(&self, d: &mut RaylibDrawHandle, tech_tree: &TechTree) {
        // Only show if tech tree is NOT open
        if tech_tree.is_open {
            return;
        }

        let screen_width = d.get_screen_width();
        let panel_width = 350;
        let panel_x = screen_width - panel_width - 10;
        let panel_y = 10;

        let current_techs = tech_tree.get_current_objectives();
        let visible_techs = current_techs.iter().take(3).collect::<Vec<_>>(); // Show max 3

        if visible_techs.is_empty() {
            return;
        }

        let panel_height = 80 + visible_techs.len() as i32 * 60;

        // Background
        d.draw_rectangle(panel_x, panel_y, panel_width, panel_height, Color::new(40, 40, 40, 200));
        d.draw_rectangle_lines(panel_x, panel_y, panel_width, panel_height, Color::WHITE);

        // Header
        d.draw_text("Current Objectives", panel_x + 10, panel_y + 10, 16, Color::WHITE);
        d.draw_text("Press T for tech tree", panel_x + 10, panel_y + 30, 12, Color::LIGHTGRAY);

        let mut current_y = panel_y + 55;

        for tech in visible_techs {
            self.draw_objective_summary(d, tech, panel_x + 10, current_y, panel_width - 20);
            current_y += 60;
        }
    }

    fn draw_objective_summary(&self, d: &mut RaylibDrawHandle, tech: &TechBox, x: i32, y: i32, width: i32) {
        // Tech name with icon
        let status_icon = match tech.status {
            TechStatus::Available => "🔓",
            TechStatus::InProgress => "⚠",
            TechStatus::Completed => "✓",
            TechStatus::Locked => "🔒",
        };
        
        let title = format!("{} {}", status_icon, tech.name);
        d.draw_text(&title, x, y, 14, Color::WHITE);

        // Progress bar
        let progress_y = y + 20;
        let progress_width = width - 50;
        let progress_height = 6;
        
        // Background
        d.draw_rectangle(x, progress_y, progress_width, progress_height, Color::DARKGRAY);
        
        // Fill
        let completion_ratio = tech.get_completion_ratio();
        let fill_width = (progress_width as f32 * completion_ratio) as i32;
        d.draw_rectangle(x, progress_y, fill_width, progress_height, tech.get_status_color());
        
        // Progress text
        let completed = tech.objectives.iter().filter(|obj| obj.completed).count();
        let total = tech.objectives.len();
        let progress_text = format!("{}/{}", completed, total);
        d.draw_text(&progress_text, x + progress_width + 5, progress_y - 2, 12, Color::LIGHTGRAY);

        // Next objective
        if let Some(next_obj) = tech.objectives.iter().find(|obj| !obj.completed) {
            let next_text = format!("Next: {}", next_obj.description);
            let truncated = if next_text.len() > 35 {
                format!("{}...", &next_text[..32])
            } else {
                next_text
            };
            d.draw_text(&truncated, x, y + 35, 11, Color::LIGHTGRAY);
        }
    }

    pub fn handle_input(&mut self, rl: &RaylibHandle, tech_tree: &mut TechTree) {
        // Toggle tech tree with T key
        if rl.is_key_pressed(KeyboardKey::KEY_T) {
            tech_tree.toggle_ui();
        }

        // Close with ESC if open
        if tech_tree.is_open && rl.is_key_pressed(KeyboardKey::KEY_ESCAPE) {
            tech_tree.is_open = false;
        }

        // Reset hovered tech each frame
        self.hovered_tech = None;
    }
}