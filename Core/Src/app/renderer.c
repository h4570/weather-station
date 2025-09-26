#include "app/renderer.h"

static void renderer_draw_div_h(lv_obj_t *parent, lv_coord_t y)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_remove_style_all(line);
    lv_obj_set_style_bg_color(line, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_size(line, 480, 1);
    lv_obj_set_pos(line, 0, y);
}

static void renderer_draw_div_v(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t h)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_remove_style_all(line);
    lv_obj_set_style_bg_color(line, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_size(line, 1, h);
    lv_obj_set_pos(line, x, y);
}

static void renderer_draw_battery(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, int level)
{
    int lv = level;
    if (lv < 0)
        lv = 0;
    if (lv > 100)
        lv = 100;
    const int segments = 4;
    const int filled = (lv * segments + 50) / 100; // zaokrąglenie

    const int32_t w_start = 70;

    // obudowa
    lv_obj_t *body = lv_obj_create(parent);
    lv_obj_remove_style_all(body);
    lv_obj_set_style_border_width(body, 1, 0);
    lv_obj_set_style_border_color(body, lv_color_black(), 0);
    lv_obj_set_size(body, w_start, 24);
    lv_obj_set_pos(body, x, y);

    // wypust
    lv_obj_t *nub = lv_obj_create(parent);
    lv_obj_remove_style_all(nub);
    lv_obj_set_style_border_width(nub, 1, 0);
    lv_obj_set_style_border_color(nub, lv_color_black(), 0);
    lv_obj_set_size(nub, 6, 10);
    lv_obj_set_pos(nub, x + w_start + 2, y + (24 - 10) / 2);

    int seg_w = 11, seg_h = 16;
    int left = x + 1 + 4; // padding wewnętrzny
    int top = y + (24 - seg_h) / 2;
    for (int i = 0; i < segments; i++)
    {
        lv_obj_t *seg = lv_obj_create(parent);
        lv_obj_remove_style_all(seg);
        lv_obj_set_style_border_width(seg, 1, 0);
        lv_obj_set_style_border_color(seg, lv_color_black(), 0);
        lv_obj_set_style_bg_color(seg, (i < filled) ? lv_color_black() : lv_color_white(), 0);
        lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
        lv_obj_set_size(seg, seg_w, seg_h);
        lv_obj_set_pos(seg, left + i * (seg_w + 5), top);
    }

    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text_fmt(lbl, "%d%%", lv);
    lv_obj_set_style_text_font(lbl, &lv_font_opensans_thin_14, 0);
    lv_obj_set_style_text_color(lbl, lv_color_black(), 0);
    lv_obj_set_pos(lbl, x + w_start + 12, y + (24 - lv_font_get_line_height(&lv_font_opensans_thin_14)) / 2);
}

static void renderer_draw_humidity_row(lv_obj_t *parent,
                                       lv_coord_t x, lv_coord_t y, lv_coord_t w, const char *value, const char *unit)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, &lv_font_opensans_thin_14, 0);
    lv_label_set_text(lbl, "WILGOTNOŚĆ");
    lv_obj_set_pos(lbl, x, y);

    lv_obj_t *val = lv_label_create(parent);
    lv_obj_set_style_text_font(val, &lv_font_opensans_regular_24, 0);
    lv_label_set_text(val, value);

    lv_obj_t *u = lv_label_create(parent);
    lv_obj_set_style_text_font(u, &lv_font_opensans_thin_14, 0);
    lv_label_set_text(u, unit);

    lv_coord_t val_w = lv_obj_get_self_width(val);
    lv_coord_t unit_w = lv_obj_get_self_width(u);
    lv_coord_t gap = 6;
    lv_obj_set_pos(val, x + w - (val_w + gap + unit_w), y - 5);
    lv_obj_set_pos(u, x + w - unit_w, y + (lv_font_get_line_height(&lv_font_opensans_regular_24) - lv_font_get_line_height(&lv_font_opensans_thin_14)) - 10);
}

static void renderer_draw_pressure_row(lv_obj_t *parent,
                                       lv_coord_t x, lv_coord_t y, lv_coord_t w, const int32_t value)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, &lv_font_opensans_thin_14, 0);
    lv_label_set_text(lbl, "CIŚNIENIE");
    lv_obj_set_pos(lbl, x, y);

    lv_obj_t *val = lv_label_create(parent);
    lv_obj_set_style_text_font(val, &lv_font_opensans_regular_24, 0);
    if (value < 98000)
        lv_label_set_text(val, "b. niskie");
    else if (value < 100000)
        lv_label_set_text(val, "niskie");
    else if (value < 102000)
        lv_label_set_text(val, "normalne");
    else if (value < 104000)
        lv_label_set_text(val, "wysokie");
    else
        lv_label_set_text(val, "b. wysokie");

    lv_coord_t val_w = lv_obj_get_self_width(val);
    lv_obj_set_pos(val, x + w - val_w, y - 5);
}

static void renderer_draw_temp_row(lv_obj_t *parent, lv_coord_t center_x, lv_coord_t baseline_y, const char *value, const char *unit)
{
    lv_obj_t *val = lv_label_create(parent);
    lv_obj_set_style_text_font(val, &lv_font_opensans_bold_numbers_72, 0);
    lv_label_set_text(val, value);
    lv_coord_t w = lv_obj_get_self_width(val);
    lv_obj_set_pos(val, center_x - w / 2, baseline_y - lv_font_get_line_height(&lv_font_opensans_bold_numbers_72));

    lv_obj_t *u = lv_label_create(parent);
    lv_obj_set_style_text_font(u, &lv_font_opensans_thin_14, 0);
    lv_label_set_text(u, unit);
    lv_obj_set_pos(u, center_x + w / 2 + 8, baseline_y - lv_font_get_line_height(&lv_font_opensans_thin_14) - 14);
}

void renderer_execute(
    float t_in, float h_in, int32_t p_in, int batt_in,
    float t_out, float h_out, int32_t p_out, int batt_out)
{
    lv_obj_t *scr = lv_screen_active();

    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *frame = lv_obj_create(scr);
    lv_obj_remove_style_all(frame);
    lv_obj_set_style_border_width(frame, 2, 0);
    lv_obj_set_style_border_color(frame, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_TRANSP, 0);
    lv_obj_set_size(frame, 480, 280);
    lv_obj_set_pos(frame, 0, 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_obj_set_style_text_font(title, &lv_font_opensans_regular_16, 0);
    lv_label_set_text(title, "STACJA POGODOWA");
    lv_obj_set_pos(title, 8, 8);

    // Warto kiedyś wykorzystać w lepszym celu
    lv_obj_t *res = lv_label_create(scr);
    lv_obj_set_style_text_font(res, &lv_font_opensans_thin_14, 0);
    lv_label_set_text(res, "Kwidzyn, Polska");
    lv_coord_t res_w = lv_obj_get_self_width(res);
    lv_obj_set_pos(res, 480 - res_w - 8, 9);

    renderer_draw_div_h(scr, 32);

    renderer_draw_div_v(scr, 240, 32, 280 - 32 - 48);

    lv_obj_t *in_lbl = lv_label_create(scr);
    lv_obj_set_style_text_font(in_lbl, &lv_font_opensans_thin_14, 0);
    lv_label_set_text(in_lbl, "WEWNĄTRZ");
    lv_obj_set_pos(in_lbl, 240 / 2 - lv_obj_get_self_width(in_lbl) / 2, 40);

    lv_obj_t *out_lbl = lv_label_create(scr);
    lv_obj_set_style_text_font(out_lbl, &lv_font_opensans_thin_14, 0);
    lv_label_set_text(out_lbl, "NA ZEWNĄTRZ");
    lv_obj_set_pos(out_lbl, 240 + 240 / 2 - lv_obj_get_self_width(out_lbl) / 2, 40);

    char buf[32];
    lv_snprintf(buf, sizeof(buf), "%.1f", t_in);
    renderer_draw_temp_row(scr, 120, 150, buf, "°C");

    lv_snprintf(buf, sizeof(buf), "%.1f", t_out);
    renderer_draw_temp_row(scr, 240 + 120, 150, buf, "°C");

    renderer_draw_div_h(scr, 160);

    lv_snprintf(buf, sizeof(buf), "%d", (int)h_in);
    renderer_draw_humidity_row(scr, 12, 170, 216, buf, "%");

    lv_snprintf(buf, sizeof(buf), "%d", (int)h_out);
    renderer_draw_humidity_row(scr, 240 + 12, 170, 216, buf, "%");

    renderer_draw_div_h(scr, 195);

    renderer_draw_pressure_row(scr, 12, 205, 216, p_in);

    renderer_draw_pressure_row(scr, 240 + 12, 205, 216, p_out);

    renderer_draw_div_h(scr, 230);

    renderer_draw_battery(scr, 12, 280 - 40, batt_in);
    renderer_draw_div_v(scr, 240, 280 - 48, 48);
    renderer_draw_battery(scr, 240 + 12, 280 - 40, batt_out);
}