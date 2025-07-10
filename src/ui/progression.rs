use crate::progression::{ProgressionSystem, Task};
use raylib::prelude::*;

pub struct ProgressionUI {
    pub panel_width: i32,
    pub panel_height: i32,
    pub margin: i32,
}

impl ProgressionUI {
    pub fn new() -> Self {
        Self {
            panel_width: 320,
            panel_height: 400,
            margin: 10,
        }
    }

    pub fn draw(&self, d: &mut RaylibDrawHandle, progression: &ProgressionSystem) {
        let screen_width = d.get_screen_width();
        let screen_height = d.get_screen_height();
        
        // Position in top-right corner
        let panel_x = screen_width - self.panel_width - self.margin;
        let panel_y = self.margin;
        
        // Draw background panel
        d.draw_rectangle(
            panel_x, 
            panel_y, 
            self.panel_width, 
            self.panel_height, 
            Color::new(40, 40, 40, 200)
        );
        
        // Draw border
        d.draw_rectangle_lines(
            panel_x, 
            panel_y, 
            self.panel_width, 
            self.panel_height, 
            Color::WHITE
        );
        
        let mut y_offset = panel_y + 10;
        
        // Draw tier title
        let tier_name = progression.current_tier.get_name();
        let tier_color = progression.current_tier.get_color();
        
        d.draw_text(
            tier_name,
            panel_x + 10,
            y_offset,
            20,
            tier_color
        );
        y_offset += 30;
        
        // Draw tier progress
        let (completed, total) = progression.get_tier_progress();
        let progress_text = format!("Progress: {}/{}", completed, total);
        d.draw_text(
            &progress_text,
            panel_x + 10,
            y_offset,
            16,
            Color::LIGHTGRAY
        );
        y_offset += 25;
        
        // Draw progress bar
        let progress_bar_width = self.panel_width - 20;
        let progress_bar_height = 8;
        let progress_ratio = if total > 0 { completed as f32 / total as f32 } else { 0.0 };
        
        // Background
        d.draw_rectangle(
            panel_x + 10,
            y_offset,
            progress_bar_width,
            progress_bar_height,
            Color::DARKGRAY
        );
        
        // Fill
        d.draw_rectangle(
            panel_x + 10,
            y_offset,
            (progress_bar_width as f32 * progress_ratio) as i32,
            progress_bar_height,
            tier_color
        );
        
        y_offset += 20;
        
        // Draw separator
        d.draw_line(
            panel_x + 10,
            y_offset,
            panel_x + self.panel_width - 10,
            y_offset,
            Color::GRAY
        );
        y_offset += 15;
        
        // Draw current tasks
        d.draw_text(
            "Current Tasks:",
            panel_x + 10,
            y_offset,
            16,
            Color::WHITE
        );
        y_offset += 25;
        
        let tasks = progression.get_incomplete_tasks();
        let max_tasks_shown = 8; // Limit visible tasks to fit in panel
        
        for (i, task) in tasks.iter().take(max_tasks_shown).enumerate() {
            let task_text = task.get_progress_text();
            let task_color = if task.completed {
                Color::GREEN
            } else {
                Color::LIGHTGRAY
            };
            
            // Wrap long task descriptions
            let max_chars = 35;
            let wrapped_text = if task_text.len() > max_chars {
                format!("{}...", &task_text[..max_chars.min(task_text.len())])
            } else {
                task_text
            };
            
            d.draw_text(
                &wrapped_text,
                panel_x + 15,
                y_offset,
                14,
                task_color
            );
            y_offset += 18;
        }
        
        // Show "and X more" if there are more tasks
        if tasks.len() > max_tasks_shown {
            let remaining = tasks.len() - max_tasks_shown;
            d.draw_text(
                &format!("... and {} more", remaining),
                panel_x + 15,
                y_offset,
                12,
                Color::GRAY
            );
        }
        
        // Draw completion status if all tasks done
        if completed == total && total > 0 {
            y_offset += 30;
            d.draw_text(
                "Tier Complete!",
                panel_x + 10,
                y_offset,
                18,
                Color::GREEN
            );
            
            if let Some(next_tier) = progression.current_tier.next() {
                y_offset += 25;
                d.draw_text(
                    &format!("Next: {}", next_tier.get_name()),
                    panel_x + 10,
                    y_offset,
                    14,
                    Color::YELLOW
                );
            } else {
                y_offset += 25;
                d.draw_text(
                    "All Tiers Complete!",
                    panel_x + 10,
                    y_offset,
                    14,
                    Color::GOLD
                );
            }
        }
    }

    pub fn draw_minimal(&self, d: &mut RaylibDrawHandle, progression: &ProgressionSystem) {
        let screen_width = d.get_screen_width();
        
        // Minimal version - just show tier and progress in corner
        let panel_x = screen_width - 250;
        let panel_y = 10;
        
        let tier_name = progression.current_tier.get_name();
        let tier_color = progression.current_tier.get_color();
        let (completed, total) = progression.get_tier_progress();
        
        let text = format!("{}: {}/{}", tier_name, completed, total);
        
        // Draw background
        let text_width = text.len() as i32 * 8; // Rough estimate
        d.draw_rectangle(
            panel_x - 5,
            panel_y - 5,
            text_width + 10,
            25,
            Color::new(0, 0, 0, 150)
        );
        
        d.draw_text(
            &text,
            panel_x,
            panel_y,
            16,
            tier_color
        );
    }
}