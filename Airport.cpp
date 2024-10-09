#include "Airport.h"
#include "Utils.h"
#include <math.h>
#include <limits.h>

Airport::Airport(sf::RenderWindow* app, sf::Font* font, float global_ds,
                 float runway_width, sf::Color runway_color,
                 float taxiway_width, sf::Color taxiway_color, float gate_length,
                 float gate_width, float gate_display_length, sf::Color gate_color, bool mode)
  : app_(app),
    font_(font),
    global_ds_(global_ds),
    runway_width_(runway_width),
    runway_color_(runway_color),
    taxiway_width_(taxiway_width),
    taxiway_color_(taxiway_color),
    gate_length_(gate_length),
    gate_width_(gate_width),
    gate_display_length_(gate_display_length),
    gate_color_(gate_color),
    mode_(mode) {

  large_gate_width_ = float(gate_width_ * 1.5);
  large_gate_length_ = float(gate_length_ * 1.5);
  large_gate_display_length_ = float(gate_display_length_ * 1.5);

  // taxiway 2 length calculation:
  float rrr = 60;
  float thetaa = 80;
  float thetaa_radian = thetaa * PI / 180.f;
  float ll = rrr - cos(thetaa_radian) * rrr;
  float hh = 100 - rrr * sin(thetaa_radian);
  float lll = tan(thetaa_radian) * hh + ll + taxiway_width / 2;
  float final_t2_len = 2700 - 280 - lll;

  RunwayDetailsParam details_param;
  details_param.num_of_threshold_markings = 12;
  details_param.threshold_marking_length = 30;
  details_param.threshold_marking_width = 2;
  details_param.threshold_marking_out_interval = 2;
  details_param.threshold_marking_to_runway_end_distance = 5;

  details_param.center_line_length = 30;
  details_param.center_line_width = 2;
  details_param.center_line_to_runway_end_distance = 100;

  details_param.num_of_touch_down_indicator = 6;
  details_param.touch_down_indicator_length = 20;
  details_param.touch_down_indicator_width = 3;
  details_param.touch_down_indicator_to_runway_end_distance = 150;
  details_param.touch_down_indicator_interval = 150;
  details_param.touch_down_indicator_main_number = 3;
  details_param.touch_down_indicator_main_to_normal_ratio = 3;

  details_param.runway_number_character_size = 30;
  details_param.runway_number_to_runway_end_distance = 100;
  details_param.runway_number_letter_to_runway_end_distance = 70;

  AddRunway({global_ds, -30.0, 2700.0, runway_width_, RouteType::RUNWAY,
             sf::Vector2f(-1300, 100), "R1", runway_color, 30}, details_param, "R");
  calling_name_to_internal_name_["12R"] = "+R1";
  calling_name_to_internal_name_["30L"] = "-R1";
  AddArcway({global_ds, -30.0, true, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("R1")->GetBreakOutPosition(2420)),
             "C1", taxiway_color, 10});
  Connect("R1", true, 2420, "C1", false, 0);
  AddTaxiway({global_ds, 60.0, 90.0, taxiway_width, RouteType::TAXIWAY,
              sf::Vector2f(GetRoutePtr("C1")->GetEndPosition()), "T1", taxiway_color, 20});
  Connect("C1", true, GetRoutePtr("C1")->GetLength(), "T1", false, 0);

  AddArcway({global_ds, 60.0, true, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("T1")->GetEndPosition()), "C2",
             taxiway_color, 10});
  Connect("T1", true, GetRoutePtr("T1")->GetLength(), "C2", false, 0);
  AddTaxiway({global_ds, 150.0, final_t2_len, taxiway_width, RouteType::TAXIWAY,
              sf::Vector2f(GetRoutePtr("C2")->GetEndPosition()), "T2", taxiway_color,
              20});
  Connect("C2", true, GetRoutePtr("C2")->GetLength(), "T2", false, 0);

  AddTaxiway({global_ds, 150 + 90 - thetaa, hh / cos(thetaa_radian),
              taxiway_width, RouteType::TAXIWAY,
              sf::Vector2f(GetRoutePtr("T2")->GetEndPosition()), "T2_1", taxiway_color,
              20});
  Connect("T2", true, GetRoutePtr("T2")->GetLength(), "T2_1", false, 0);
  AddArcway({global_ds, 150.0, false, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("T2")->GetBreakOutPosition(95)),
             "C2_1", taxiway_color, 10});
  Connect("T2", true, 95, "C2_1", false, 0);
  AddTaxiway({global_ds, 60.0, 300.0, taxiway_width, RouteType::TAXIWAY,
              sf::Vector2f(GetRoutePtr("C2_1")->GetEndPosition()), "TP", taxiway_color,
              30});
  Connect("C2_1", true, GetRoutePtr("C2_1")->GetLength(), "TP", false,
          0);
  AddArcway({global_ds, -30.0, true, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("T2")->GetBreakOutPosition(175)), "C2_2",
             taxiway_color, 10});
  Connect("C2_2", true, GetRoutePtr("C2_2")->GetLength(), "TP", false,
          0);
  Connect("T2", false, 175, "C2_2", false, 0);

  // Additional structure to conventional Kingston-Norman Manley Airport.
  AddArcway({global_ds, 60.0, true, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("TP")->GetEndPosition()),
             "C5_1", taxiway_color, 10});
  Connect("TP", true, GetRoutePtr("TP")->GetLength(), "C5_1", false, 0);
  AddTaxiway({global_ds, 150.0, 2100, taxiway_width, RouteType::TAXIWAY,
              sf::Vector2f(GetRoutePtr("C5_1")->GetEndPosition()), "T5", taxiway_color,
              20});
  Connect("C5_1", true, GetRoutePtr("C5_1")->GetLength(), "T5", false, 0);
  AddArcway({global_ds, 150, false, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("T5")->GetEndPosition()),
             "C6", taxiway_color, 10});
  Connect("T5", true, GetRoutePtr("T5")->GetLength(), "C6", false, 0);
  AddTaxiway({global_ds, 60, 100, taxiway_width, RouteType::TAXIWAY,
              sf::Vector2f(GetRoutePtr("C6")->GetEndPosition()), "T6", taxiway_color,
              20});
  Connect("C6", true, GetRoutePtr("C6")->GetLength(), "T6", false, 0);
  AddArcway({global_ds, 60, false, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("T6")->GetEndPosition()),
             "C7", taxiway_color, 10});
  Connect("T6", true, GetRoutePtr("T6")->GetLength(), "C7", false, 0);
  AddRunway({global_ds, -30.0, 2700.0, runway_width_, RouteType::RUNWAY,
            GetRoutePtr("C7")->GetEndPosition()+ToSfmlPosition(sf::Vector2f(-51/2*sqrt(3), -51/2)), "R2", runway_color, 30}, details_param, "L");
  calling_name_to_internal_name_["12L"] = "+R2";
  calling_name_to_internal_name_["30R"] = "-R2";

  Connect("C7", true, GetRoutePtr("C7")->GetLength(), "R2", false, 51);
  AddArcway({global_ds, 60, false, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("TP")->GetEndPosition()),
             "C5_2", taxiway_color, 10});
  Connect("TP", true, GetRoutePtr("TP")->GetLength(), "C5_2", false, 0);
  AddTaxiway({global_ds, -30, 400, taxiway_width, RouteType::TAXIWAY,
              sf::Vector2f(GetRoutePtr("C5_2")->GetEndPosition()), "T5_1", taxiway_color,
              20});
  Connect("C5_2", true, GetRoutePtr("C5_2")->GetLength(), "T5_1", false, 0);
  AddArcway({global_ds, -30, true, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("T5_1")->GetEndPosition()),
             "C8", taxiway_color, 10});
  Connect("T5_1", true, GetRoutePtr("T5_1")->GetLength(), "C8", false, 0);
  AddTaxiway({global_ds, -30, 80, taxiway_width, RouteType::TAXIWAY,
              sf::Vector2f(GetRoutePtr("T5")->GetStartPosition()), "T5_2", taxiway_color,
              20});
  Connect("T5", false, 0, "T5_2", false, 0);
  Connect("T5_2", true, GetRoutePtr("T5_2")->GetLength(), "T5_1", false, 0);

  AddTaxiway({global_ds, 60, 100, taxiway_width, RouteType::TAXIWAY,
              sf::Vector2f(GetRoutePtr("C8")->GetEndPosition()), "T8", taxiway_color,
              20});
  Connect("C8", true, GetRoutePtr("C8")->GetLength(), "T8", false, 0);
  AddArcway({global_ds, 60, true, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("T8")->GetEndPosition()),
             "C9", taxiway_color, 10});
  Connect("T8", true, GetRoutePtr("T8")->GetLength(), "C9", false, 0);
  Connect("C9", true, GetRoutePtr("C9")->GetLength(), "R2", true, 2631); // 2700-51-2100-80-400

  // Additional gates
  AddArcway({global_ds, 150.0, false, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("T2")->GetBreakOutPosition(395)),
             "C2_3", taxiway_color, 10});
  Connect("T2", true, 395, "C2_3", false, 0);

  AddTaxiway({global_ds, 60.0, 300.0, taxiway_width, RouteType::TAXIWAY,
              sf::Vector2f(GetRoutePtr("C2_3")->GetEndPosition()), "TP_1", taxiway_color,
              30});
  Connect("C2_3", true, GetRoutePtr("C2_3")->GetLength(), "TP_1", false,
          0);
  AddArcway({global_ds, -30.0, true, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("T2")->GetBreakOutPosition(475)), "C2_4",
             taxiway_color, 10});
  Connect("C2_4", true, GetRoutePtr("C2_4")->GetLength(), "TP_1", false,
          0);
  Connect("T2", false, 475, "C2_4", false, 0);
  AddArcway({global_ds, 60.0, true, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("TP_1")->GetEndPosition()),
             "C5_3", taxiway_color, 10});
  Connect("TP_1", true, GetRoutePtr("TP_1")->GetLength(), "C5_3", false, 0);
  AddArcway({global_ds, 60, false, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("TP_1")->GetEndPosition()),
             "C5_4", taxiway_color, 10});
  Connect("TP_1", true, GetRoutePtr("TP_1")->GetLength(), "C5_4", false, 0);
  Connect("C5_3", true, GetRoutePtr("C5_3")->GetLength(), "T5", false, 300);
  Connect("C5_4", true, GetRoutePtr("C5_4")->GetLength(), "T5", true, 220);

  // Additional gates code goes here
  AddArcway({global_ds, 60.0, true, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(225)), "CP_21",
             taxiway_color, 10});
  Connect("CP_21", false, 0, "TP_1", true, 225);
  AddArcway({global_ds, -120.0, false, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(285)), "CP_22",
             taxiway_color, 10});
  Connect("CP_22", false, 0, "TP_1", false, 285);
  AddGate({global_ds, 150.0, gate_length, gate_width, gate_display_length,
           RouteType::GATE, sf::Vector2f(GetRoutePtr("CP_21")->GetEndPosition()), "G8",
           gate_color, 1.2, 2});
  gates_.back()->AddPushBackRoute("+R1", GetRoutePtr("CP_22"));
  gates_.back()->AddPushBackRoute("-R1", GetRoutePtr("CP_22"));
  gates_.back()->AddPushBackRoute("+R2", GetRoutePtr("CP_21"));
  gates_.back()->AddPushBackRoute("-R2", GetRoutePtr("CP_21"));
  Connect("CP_21", true, GetRoutePtr("CP_21")->GetLength(), "G8", false, 0);
  Connect("CP_22", true, GetRoutePtr("CP_22")->GetLength(), "G8", false, 0);

  AddArcway({global_ds, 60.0, true, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(165)), "CP_23",
             taxiway_color, 10});
  AddArcway({global_ds, -120.0, false, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(225)), "CP_24",
             taxiway_color, 10});
  Connect("CP_23", false, 0, "TP_1", true, 165);
  Connect("CP_24", false, 0, "TP_1", false, 225);

  AddGate({global_ds, 150.0, gate_length, gate_width, gate_display_length,
           RouteType::GATE, sf::Vector2f(GetRoutePtr("CP_23")->GetEndPosition()), "G9",
           gate_color, 1.2, 2});
  gates_.back()
      ->AddPushBackRoute("+R1", GetRoutePtr("CP_24"));
  gates_.back()
      ->AddPushBackRoute("-R1", GetRoutePtr("CP_24"));
  gates_.back()
      ->AddPushBackRoute("+R2", GetRoutePtr("CP_23"));
  gates_.back()
      ->AddPushBackRoute("-R2", GetRoutePtr("CP_23"));
  Connect("CP_23", true, GetRoutePtr("CP_23")->GetLength(), "G9", false, 0);
  Connect("CP_24", true, GetRoutePtr("CP_24")->GetLength(), "G9", false, 0);

  AddArcway({global_ds, 60.0, true, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(105)), "CP_25",
             taxiway_color, 10});
  AddArcway({global_ds, -120.0, false, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(165)), "CP_26",
             taxiway_color, 10});
  Connect("CP_25", false, 0, "TP_1", true, 105);
  Connect("CP_26", false, 0, "TP_1", false, 165);
  AddGate({global_ds, 150.0, gate_length, gate_width, gate_display_length,
           RouteType::GATE, sf::Vector2f(GetRoutePtr("CP_25")->GetEndPosition()),
           "G10", gate_color, 1.2, 2});
  gates_.back()->AddPushBackRoute("+R1", GetRoutePtr("CP_26"));
  gates_.back()->AddPushBackRoute("-R1", GetRoutePtr("CP_26"));
  gates_.back()->AddPushBackRoute("+R2", GetRoutePtr("CP_25"));
  gates_.back()->AddPushBackRoute("-R2", GetRoutePtr("CP_25"));
  Connect("CP_25", true, GetRoutePtr("CP_25")->GetLength(), "G10", false, 0);
  Connect("CP_26", true, GetRoutePtr("CP_26")->GetLength(), "G10", false, 0);

  AddArcway({global_ds, 60.0, true, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(45)), "CP_27",
             taxiway_color, 10});
  AddArcway({global_ds, -120.0, false, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(105)), "CP_28",
             taxiway_color, 10});
  Connect("CP_27", false, 0, "TP_1", true, 45);
  Connect("CP_28", false, 0, "TP_1", false, 105);
  AddGate({global_ds, 150.0, gate_length, gate_width, gate_display_length,
           RouteType::GATE, sf::Vector2f(GetRoutePtr("CP_27")->GetEndPosition()),
           "G11", gate_color, 1.2, 2});
  gates_.back()->AddPushBackRoute("+R1", GetRoutePtr("CP_28"));
  gates_.back()->AddPushBackRoute("-R1", GetRoutePtr("CP_28"));
  gates_.back()->AddPushBackRoute("+R2", GetRoutePtr("CP_27"));
  gates_.back()->AddPushBackRoute("-R2", GetRoutePtr("CP_27"));
  Connect("CP_27", true, GetRoutePtr("CP_27")->GetLength(), "G11", false, 0);
  Connect("CP_28", true, GetRoutePtr("CP_28")->GetLength(), "G11", false, 0);

  // Add large gate on right side of TP
  AddArcway({global_ds, 60, false, large_gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(20)), "CP_29",
             taxiway_color, 10});
  AddArcway({global_ds, -120.0, true, large_gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(110)), "CP_30",
             taxiway_color, 10});
  Connect("CP_29", false, 0, "TP_1", true, 20);
  Connect("CP_30", false, 0, "TP_1", false, 110);
  AddGate({global_ds, -30, large_gate_length_, large_gate_width_, large_gate_display_length_,
           RouteType::GATE, sf::Vector2f(GetRoutePtr("CP_29")->GetEndPosition()),
           "G12", gate_color, 1.2, 3});
  gates_.back()->AddPushBackRoute("+R1", GetRoutePtr("CP_30"));
  gates_.back()->AddPushBackRoute("-R1", GetRoutePtr("CP_30"));
  gates_.back()->AddPushBackRoute("+R2", GetRoutePtr("CP_29"));
  gates_.back()->AddPushBackRoute("-R2", GetRoutePtr("CP_29"));
  Connect("CP_29", true, GetRoutePtr("CP_29")->GetLength(), "G12", false, 0);
  Connect("CP_30", true, GetRoutePtr("CP_30")->GetLength(), "G12", false, 0);

  AddArcway({global_ds, 60, false, large_gate_width_/2, 90.0, taxiway_width, RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(110)), "CP_31",
             taxiway_color, 10});
  AddArcway({global_ds, -120.0, true, large_gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(200)), "CP_32",
             taxiway_color, 10});
  Connect("CP_31", false, 0, "TP_1", true, 110);
  Connect("CP_32", false, 0, "TP_1", false, 200);
  AddGate({global_ds, -30, large_gate_length_, large_gate_width_,
           large_gate_display_length_, RouteType::GATE,
           sf::Vector2f(GetRoutePtr("CP_31")->GetEndPosition()), "G13",
           gate_color, 1.2, 3});
  gates_.back()->AddPushBackRoute("+R1", GetRoutePtr("CP_32"));
  gates_.back()->AddPushBackRoute("-R1", GetRoutePtr("CP_32"));
  gates_.back()->AddPushBackRoute("+R2", GetRoutePtr("CP_31"));
  gates_.back()->AddPushBackRoute("-R2", GetRoutePtr("CP_31"));
  Connect("CP_31", true, GetRoutePtr("CP_31")->GetLength(), "G13", false, 0);
  Connect("CP_32", true, GetRoutePtr("CP_32")->GetLength(), "G13", false, 0);

  AddArcway({global_ds, 60, false, large_gate_width_/2, 90.0, taxiway_width, RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(200)), "CP_33",
             taxiway_color, 10});
  AddArcway({global_ds, -120.0, true, large_gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP_1")->GetBreakOutPosition(290)), "CP_34",
             taxiway_color, 10});
  Connect("CP_33", false, 0, "TP_1", true, 200);
  Connect("CP_34", false, 0, "TP_1", false, 290);
  AddGate({global_ds, -30, large_gate_length_, large_gate_width_,
           large_gate_display_length_, RouteType::GATE,
           sf::Vector2f(GetRoutePtr("CP_33")->GetEndPosition()), "G14",
           gate_color, 1.2, 3});
  gates_.back()->AddPushBackRoute("+R1", GetRoutePtr("CP_34"));
  gates_.back()->AddPushBackRoute("-R1", GetRoutePtr("CP_34"));
  gates_.back()->AddPushBackRoute("+R2", GetRoutePtr("CP_33"));
  gates_.back()->AddPushBackRoute("-R2", GetRoutePtr("CP_33"));
  Connect("CP_33", true, GetRoutePtr("CP_33")->GetLength(), "G14", false, 0);
  Connect("CP_34", true, GetRoutePtr("CP_34")->GetLength(), "G14", false, 0);
  // Additional gates code ends here

  // Gates 1-5
  AddArcway({global_ds, 60.0, true, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(225)), "CP_1",
             taxiway_color, 10});
  Connect("CP_1", false, 0, "TP", true, 225);
  AddArcway({global_ds, -120.0, false, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(285)), "CP_2",
             taxiway_color, 10});
  Connect("CP_2", false, 0, "TP", false, 285);
  AddGate({global_ds, 150.0, gate_length, gate_width, gate_display_length,
           RouteType::GATE, sf::Vector2f(GetRoutePtr("CP_1")->GetEndPosition()), "G1",
           gate_color, 1.2, 2});
  gates_.back()->AddPushBackRoute("+R1", GetRoutePtr("CP_2"));
  gates_.back()->AddPushBackRoute("-R1", GetRoutePtr("CP_2"));
  gates_.back()->AddPushBackRoute("+R2", GetRoutePtr("CP_1"));
  gates_.back()->AddPushBackRoute("-R2", GetRoutePtr("CP_1"));
  Connect("CP_1", true, GetRoutePtr("CP_1")->GetLength(), "G1", false, 0);
  Connect("CP_2", true, GetRoutePtr("CP_2")->GetLength(), "G1", false, 0);

  AddArcway({global_ds, 60.0, true, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(165)), "CP_3",
             taxiway_color, 10});
  AddArcway({global_ds, -120.0, false, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(225)), "CP_4",
             taxiway_color, 10});
  Connect("CP_3", false, 0, "TP", true, 165);
  Connect("CP_4", false, 0, "TP", false, 225);

  AddGate({global_ds, 150.0, gate_length, gate_width, gate_display_length,
           RouteType::GATE, sf::Vector2f(GetRoutePtr("CP_3")->GetEndPosition()), "G2",
           gate_color, 1.2, 2});
  gates_.back()
      ->AddPushBackRoute("+R1", GetRoutePtr("CP_4"));
  gates_.back()
      ->AddPushBackRoute("-R1", GetRoutePtr("CP_4"));
  gates_.back()
      ->AddPushBackRoute("+R2", GetRoutePtr("CP_3"));
  gates_.back()
      ->AddPushBackRoute("-R2", GetRoutePtr("CP_3"));
  Connect("CP_3", true, GetRoutePtr("CP_3")->GetLength(), "G2", false, 0);
  Connect("CP_4", true, GetRoutePtr("CP_4")->GetLength(), "G2", false, 0);

  AddArcway({global_ds, 60.0, true, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(105)), "CP_5",
             taxiway_color, 10});
  AddArcway({global_ds, -120.0, false, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(165)), "CP_6",
             taxiway_color, 10});
  Connect("CP_5", false, 0, "TP", true, 105);
  Connect("CP_6", false, 0, "TP", false, 165);
  AddGate({global_ds, 150.0, gate_length, gate_width, gate_display_length,
           RouteType::GATE, sf::Vector2f(GetRoutePtr("CP_5")->GetEndPosition()),
           "G3", gate_color, 1.2, 2});
  gates_.back()->AddPushBackRoute("+R1", GetRoutePtr("CP_6"));
  gates_.back()->AddPushBackRoute("-R1", GetRoutePtr("CP_6"));
  gates_.back()->AddPushBackRoute("+R2", GetRoutePtr("CP_5"));
  gates_.back()->AddPushBackRoute("-R2", GetRoutePtr("CP_5"));
  Connect("CP_5", true, GetRoutePtr("CP_5")->GetLength(), "G3", false, 0);
  Connect("CP_6", true, GetRoutePtr("CP_6")->GetLength(), "G3", false, 0);

  AddArcway({global_ds, 60.0, true, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(45)), "CP_7",
             taxiway_color, 10});
  AddArcway({global_ds, -120.0, false, gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(105)), "CP_8",
             taxiway_color, 10});
  Connect("CP_7", false, 0, "TP", true, 45);
  Connect("CP_8", false, 0, "TP", false, 105);
  AddGate({global_ds, 150.0, gate_length, gate_width, gate_display_length,
           RouteType::GATE, sf::Vector2f(GetRoutePtr("CP_7")->GetEndPosition()),
           "G4", gate_color, 1.2, 2});
  gates_.back()->AddPushBackRoute("+R1", GetRoutePtr("CP_8"));
  gates_.back()->AddPushBackRoute("-R1", GetRoutePtr("CP_8"));
  gates_.back()->AddPushBackRoute("+R2", GetRoutePtr("CP_7"));
  gates_.back()->AddPushBackRoute("-R2", GetRoutePtr("CP_7"));
  Connect("CP_7", true, GetRoutePtr("CP_7")->GetLength(), "G4", false, 0);
  Connect("CP_8", true, GetRoutePtr("CP_8")->GetLength(), "G4", false, 0);

  // Add large gate on right side of TP
  AddArcway({global_ds, 60, false, large_gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(20)), "CP_9",
             taxiway_color, 10});
  AddArcway({global_ds, -120.0, true, large_gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(110)), "CP_10",
             taxiway_color, 10});
  Connect("CP_9", false, 0, "TP", true, 20);
  Connect("CP_10", false, 0, "TP", false, 110);
  AddGate({global_ds, -30, large_gate_length_, large_gate_width_, large_gate_display_length_,
           RouteType::GATE, sf::Vector2f(GetRoutePtr("CP_9")->GetEndPosition()),
           "G5", gate_color, 1.2, 3});
  gates_.back()->AddPushBackRoute("+R1", GetRoutePtr("CP_10"));
  gates_.back()->AddPushBackRoute("-R1", GetRoutePtr("CP_10"));
  gates_.back()->AddPushBackRoute("+R2", GetRoutePtr("CP_9"));
  gates_.back()->AddPushBackRoute("-R2", GetRoutePtr("CP_9"));
  Connect("CP_9", true, GetRoutePtr("CP_9")->GetLength(), "G5", false, 0);
  Connect("CP_10", true, GetRoutePtr("CP_10")->GetLength(), "G5", false, 0);

  AddArcway({global_ds, 60, false, large_gate_width_/2, 90.0, taxiway_width, RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(110)), "CP_11",
             taxiway_color, 10});
  AddArcway({global_ds, -120.0, true, large_gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(200)), "CP_12",
             taxiway_color, 10});
  Connect("CP_11", false, 0, "TP", true, 110);
  Connect("CP_12", false, 0, "TP", false, 200);
  AddGate({global_ds, -30, large_gate_length_, large_gate_width_,
           large_gate_display_length_, RouteType::GATE,
           sf::Vector2f(GetRoutePtr("CP_11")->GetEndPosition()), "G6",
           gate_color, 1.2, 3});
  gates_.back()->AddPushBackRoute("+R1", GetRoutePtr("CP_12"));
  gates_.back()->AddPushBackRoute("-R1", GetRoutePtr("CP_12"));
  gates_.back()->AddPushBackRoute("+R2", GetRoutePtr("CP_11"));
  gates_.back()->AddPushBackRoute("-R2", GetRoutePtr("CP_11"));
  Connect("CP_11", true, GetRoutePtr("CP_11")->GetLength(), "G6", false, 0);
  Connect("CP_12", true, GetRoutePtr("CP_12")->GetLength(), "G6", false, 0);

  AddArcway({global_ds, 60, false, large_gate_width_/2, 90.0, taxiway_width, RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(200)), "CP_13",
             taxiway_color, 10});
  AddArcway({global_ds, -120.0, true, large_gate_width_/2, 90.0, taxiway_width,
             RouteType::ARCWAY,
             sf::Vector2f(GetRoutePtr("TP")->GetBreakOutPosition(290)), "CP_14",
             taxiway_color, 10});
  Connect("CP_13", false, 0, "TP", true, 200);
  Connect("CP_14", false, 0, "TP", false, 290);
  AddGate({global_ds, -30, large_gate_length_, large_gate_width_,
           large_gate_display_length_, RouteType::GATE,
           sf::Vector2f(GetRoutePtr("CP_13")->GetEndPosition()), "G7",
           gate_color, 1.2, 3});
  gates_.back()->AddPushBackRoute("+R1", GetRoutePtr("CP_14"));
  gates_.back()->AddPushBackRoute("-R1", GetRoutePtr("CP_14"));
  gates_.back()->AddPushBackRoute("+R2", GetRoutePtr("CP_13"));
  gates_.back()->AddPushBackRoute("-R2", GetRoutePtr("CP_13"));
  Connect("CP_13", true, GetRoutePtr("CP_13")->GetLength(), "G7", false, 0);
  Connect("CP_14", true, GetRoutePtr("CP_14")->GetLength(), "G7", false, 0);

  AddArcway({global_ds, 150 + 90 - thetaa, true, rrr, thetaa, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("T2_1")->GetEndPosition()),
             "C3", taxiway_color, 10});
  Connect("T2_1", true, GetRoutePtr("T2_1")->GetLength(), "C3", false, 0);
  AddTaxiway({global_ds, 240.0, 30, taxiway_width, RouteType::TAXIWAY,
              sf::Vector2f(GetRoutePtr("C3")->GetEndPosition()), "T3", taxiway_color,
              20});

  AddArcway({global_ds, 240.0, true, 40.0, 90.0, taxiway_width,
             RouteType::ARCWAY, sf::Vector2f(GetRoutePtr("T3")->GetEndPosition()), "C4",
             taxiway_color, 10});
  Connect("T3", true, GetRoutePtr("T3")->GetLength(), "C4", false, 0);
  Connect("C4", true, GetRoutePtr("C4")->GetLength(), "R1", false, 51);

  Connect("C3", true, GetRoutePtr("C3")->GetLength(), "T3", false, 0);

  AddHoldPoint({GetRoutePtr("T2"), 60, HoldPointType::TRAFFIC, true});
  AddHoldPoint({GetRoutePtr("T2"), 210, HoldPointType::TRAFFIC, false});
  AddHoldPoint({GetRoutePtr("T5_1"), 35, HoldPointType::TRAFFIC, false});
  AddHoldPoint({GetRoutePtr("T5"), 35, HoldPointType::TRAFFIC, false});
  AddHoldPoint({GetRoutePtr("T5"), 335, HoldPointType::TRAFFIC, false});
  AddHoldPoint({GetRoutePtr("T2"), 510, HoldPointType::TRAFFIC, false});
  AddHoldPoint({GetRoutePtr("T5"), 185, HoldPointType::TRAFFIC, true});
  AddHoldPoint({GetRoutePtr("T2"), 360, HoldPointType::TRAFFIC, true});

  AddHoldPoint({GetRoutePtr("C3"), float(GetRoutePtr("C3")->GetLength() / 1.1), HoldPointType::TAKEOFF, true, "+R1"});
  AddHoldPoint({GetRoutePtr("T1"), GetRoutePtr("T1")->GetLength() / 2, HoldPointType::TAKEOFF, false, "-R1"});
  AddHoldPoint({GetRoutePtr("T6"), GetRoutePtr("T6")->GetLength() / 2, HoldPointType::TAKEOFF, true, "+R2"});
  AddHoldPoint({GetRoutePtr("T8"), GetRoutePtr("T8")->GetLength() / 2, HoldPointType::TAKEOFF, true, "-R2"});
  AddHoldPoint({GetRoutePtr("C4"), GetRoutePtr("C4")->GetLength(), HoldPointType::LINEUP, true, "+R1"});
  AddHoldPoint({GetRoutePtr("C7"), GetRoutePtr("C7")->GetLength(), HoldPointType::LINEUP, true, "+R2"});
  AddHoldPoint({GetRoutePtr("C1"), 0, HoldPointType::LINEUP, false, "-R1"});
  AddHoldPoint({GetRoutePtr("C9"), GetRoutePtr("C9")->GetLength(), HoldPointType::LINEUP, true, "-R2"});


  BuildConnectionMatrix();
}

void Airport::BuildConnectionMatrix() {
  // 1. build SegmentInfo
  for (auto& r : runways_) { r->CreateSegments(); }
  for (auto& t : taxiways_) { t->CreateSegments(); }
  for (auto& a : arcways_) { a->CreateSegments(); }
  for (auto& g : gates_) { g->CreateSegments(); }

  // 2. populate intra-route segment info for connections
  for (auto& r : runways_) { r->PopulateIntraRouteConnection(); }
  for (auto& t : taxiways_) { t->PopulateIntraRouteConnection(); }
  for (auto& a : arcways_) { a->PopulateIntraRouteConnection(); }
  for (auto& g : gates_) { g->PopulateIntraRouteConnection(); }

  // 3. build matrix
  // 3.1 build id2name and name2id map, and copy segments
  int num_of_segments = 0;
  for (auto& r : runways_) {
    auto segments = r->GetSegments();
    for (auto& s : segments) {
      matrix_id_to_name_.insert({num_of_segments, s.name});
      name_to_matrix_id_.insert({s.name, num_of_segments});
      num_of_segments++;
      segments_.push_back(s);
    }
  }
  for (auto& t : taxiways_) {
    auto segments = t->GetSegments();
    for (auto& s : segments) {
      matrix_id_to_name_.insert({num_of_segments, s.name});
      name_to_matrix_id_.insert({s.name, num_of_segments});
      num_of_segments++;
      segments_.push_back(s);
    }
  }
  for (auto& a : arcways_) {
    auto segments = a->GetSegments();
    for (auto& s : segments) {
      matrix_id_to_name_.insert({num_of_segments, s.name});
      name_to_matrix_id_.insert({s.name, num_of_segments});
      num_of_segments++;
      segments_.push_back(s);
    }
  }
  for (auto& g : gates_) {
    auto segments = g->GetSegments();
    for (auto& s : segments) {
      matrix_id_to_name_.insert({num_of_segments, s.name});
      name_to_matrix_id_.insert({s.name, num_of_segments});
      num_of_segments++;
      segments_.push_back(s);
    }
  }
  // std::cout << "Total number of segments of this airport is:" << num_of_segments << std::endl;
  // 3.2 build connection matrix
  connection_matrix_ = std::vector<std::vector<float>>(num_of_segments, std::vector<float>(num_of_segments, float(INT_MAX)));
  for (int i=0; i<num_of_segments; i++) {
    for (int j=0; j<num_of_segments; j++) {
      if (i==j) {connection_matrix_[i][j] = 0; continue;}
      for (auto& name : segments_[i].out_segment) {
        if (name == segments_[j].name) {
          connection_matrix_[i][j] = segments_[i].length;
          break;
        }
      }
    }
  }
  // std::cout << "Connection Matrix Finished." << std::endl;
  for (int i=0; i<connection_matrix_.size(); i++) {
    // std::cout << i << ": ";
    for (int j=0; j<connection_matrix_[i].size(); j++) {
      // std::cout << connection_matrix_[i][j] << " ";
    }
    // std::cout << std::endl;
  }
}

RouteBase* Airport::GetSegmentRoute(std::string segment_name) {
  for (auto& s : segments_) {
    if (s.name == segment_name) {
      return s.route;
    }
  }
  return nullptr;
}

// return a minimum distance for the
// vertex which is not included in visited.
int FindIndexOfMinimumValue(std::vector<float> dist, std::vector<bool> visited) {
  int n = dist.size();
  float min_dist = INT_MAX;
  int index;

  for (int i = 0; i < n; i++) {
    if (!visited[i] && dist[i] <= min_dist) {
      min_dist = dist[i];
      index = i;
    }
  }
  return index;
}

std::list<std::string> Airport::Dijkstra(std::vector<std::vector<float>> graph, int src, int dst) {
  int n = graph.size();
  std::vector<float> dist(n, INT_MAX); // integer array to calculate minimum distance for each node.
  std::vector<bool> visited(n, false);// mark visted/unvisted for each node.
  std::vector<int> from(n, -1);
  int prev = -1;

  dist[src] = 0; // Source vertex distance is set to zero.

  for (int i = 0; i < n; i++) {
    int index = FindIndexOfMinimumValue(dist, visited); // vertex not yet included.
    // std::cout << "Index picked: " << std::to_string(index) << "    ";
    prev = index;
    visited[index] = true; // m with minimum distance included in visited.
    for (int j = 0; j < n; j++) {
      // Updating the minimum distance for the particular node.
	  if (!visited[j] && (graph[index][j] > 0 && graph[index][j] < INT_MAX) && dist[index] != INT_MAX && dist[index] + graph[index][j] < dist[j]) {
        dist[j] = dist[index] + graph[index][j];
        if (from[j] == -1) {
          // std::cout << "from[" << std::to_string(j) << "] set to " << std::to_string(index);
          from[j] = index;
        }
      }
    }
    // std::cout << std::endl;
  }

  // std::cout << "Print from:" << std::endl;
  // for (int i=0; i<from.size(); i++) {
  //   std::cout << std::to_string(i) << ": " << std::to_string(from[i]) << std::endl;
  // }

  // print result:
  std::vector<int> result_id_list(0, 0);
  // std::cout << dst << ": " << dist[dst] << std::endl;
  int prev_id = dst;
  do { result_id_list.push_back(prev_id);
       prev_id = from[prev_id]; }
  while (prev_id != src);
  result_id_list.push_back(prev_id);
  std::reverse(std::begin(result_id_list), std::end(result_id_list));

  for (auto v : result_id_list) {
    // std::cout << std::to_string(v) << " ";
  }
  // std::cout << std::endl;

  std::vector<std::string> result_string_list;
  for (auto v : result_id_list) {
    result_string_list.push_back(matrix_id_to_name_[v]);
    // std::cout << result_string_list.back() << " ";
  }
  // std::cout << std::endl;

  std::list<std::string> route_list;
  for (auto v : result_id_list) {
    if (route_list.empty() ||
        (route_list.back() != GetSegmentRoute(matrix_id_to_name_[v])->GetName())) {
      route_list.push_back(GetSegmentRoute(matrix_id_to_name_[v])->GetName());
    }
  }
  return route_list;
}

std::list<std::string> Airport::GetRoute(RouteBase* start_route, bool start_direction, float start_dist,
                       RouteBase* end_route, bool end_direction, float end_dist) {
  auto name1 = start_route->GetSegmentName(start_direction, start_dist);
  auto name2 = end_route->GetSegmentName(end_direction, end_dist);

  auto id1 = name_to_matrix_id_[name1];
  auto id2 = name_to_matrix_id_[name2];
  // std::cout << "ID1 and ID2: "<< id1 << " " << id2 << std::endl;

  return Dijkstra(connection_matrix_, id1, id2);

}

void Airport::AddRunway(LineParameter param, RunwayDetailsParam details_param, std::string airport_letter) {
  runways_.push_back(new Runway(param, details_param, app_, font_, airport_letter));
  str_2_ptr_[param.name] = runways_.back();
}

void Airport::AddTaxiway(LineParameter param) {
  taxiways_.push_back(new Taxiway(param, app_, font_));
  str_2_ptr_[param.name] = taxiways_.back();
}

void Airport::AddArcway(ArcParameter param) {
  arcways_.push_back(new Arcway(param, app_, font_));
  str_2_ptr_[param.name] = arcways_.back();
}

void Airport::AddGate(GateParameter param) {
  gates_.push_back(new Gate(param, app_, font_));
  str_2_ptr_[param.name] = gates_.back();
}

void Airport::AddHoldPoint(HoldPoint hold_point) {
  hold_point.route->AddHoldPoint(hold_point);
  holdpoints_.push_back(hold_point);
}

RouteBase* Airport::GetRoutePtr(std::string route_name) {
  return str_2_ptr_.count(route_name) > 0 ? str_2_ptr_[route_name] : nullptr;
}

void Airport::Connect(std::string route_name_1, bool direction_1, float dist_1,
                      std::string route_name_2, bool direction_2, float dist_2) {
  RouteBase* first = GetRoutePtr(route_name_1);
  RouteBase* second = GetRoutePtr(route_name_2);
  if (first->AllowTravelInDirection(direction_1)) {
    first->ConnectRoute(dist_1,
      direction_1, second,
      dist_2,
      !direction_2);
  }
  if (second->AllowTravelInDirection(direction_2)) {
    second->ConnectRoute(dist_2,
      direction_2, first,
      dist_1,
      !direction_1);
  }
}

void Airport::Draw() {
  for (auto& r : taxiways_) {
    r->Draw(display_road_text_);
  }
  for (auto& r : arcways_) {
    r->Draw(display_road_text_);
  }
  for (auto& r : gates_) {
    r->Draw(true);
  }
  for (auto& r : runways_) {
    r->Draw(display_road_text_);
  }
}

std::vector<RouteBase*> Airport::GetAvailableGates() {
  std::vector<RouteBase*> res;
  for (auto& g : gates_) {
    if (g->IsAvailable()) {
      res.push_back(g);
    }
  }
  return res;
}

std::vector<RouteBase*> Airport::GetGates() {
  std::vector<RouteBase*> res;
  for (auto& g : gates_) {
    res.push_back(g);
  }
  return res;
}

std::vector<RouteBase*> Airport::GetRunways() {
  std::vector<RouteBase*> res;
  for (auto& r : runways_) {
    res.push_back(r);
  }
  return res;
}

std::vector<std::string> Airport::GetActiveRunwayStrings() {
  std::vector<std::string> res;
  for (auto& r : runways_) {
    res.push_back(r->GetRunwayCallingNames()[mode_ ? 1 : 0]);
  }
  return res;
}

std::vector<std::shared_ptr<RunwayInfo>> Airport::GetActiveRunwayInfo() {
  std::vector<std::shared_ptr<RunwayInfo>> res;
  for (auto& r : runways_) {
    for (auto& info : r->GetRunwayInfo()) {
      int runway_degree = info->runway_number * 10;
      int low_bound = runway_degree - 90;
      int up_bound = runway_degree + 90;
      if ((wind_direction_ >= low_bound && wind_direction_ < up_bound) ||
          (wind_direction_-360 >= low_bound && wind_direction_-360 < up_bound) ||
          (wind_direction_+360 >= low_bound && wind_direction_+360 < up_bound)) {
        res.push_back(info);
      }
    }
  }
  return res;
}

void Airport::FlipRoadText() {
  display_road_text_ = !display_road_text_;
}

std::list<std::string> Airport::ComputeRouteTo(RouteBase* from_route,
                                               float dist_on_from,
                                               RouteBase* to_route,
                                               float dist_on_to) {
  std::list<std::string> res;
  return res;
}

std::vector<Gate*> Airport::GetGatesWithExactSize(int size) {
  std::vector<Gate*> res;
  for (auto& g : gates_) {
    if (g->GetSize() == size) {
      res.push_back(g);
    }
  }
  return res;
}

void Airport::Reset() {
  display_road_text_ = false;
  for (auto& r : runways_) { r->Reset(); }
  for (auto& t : taxiways_) { t->Reset(); }
  for (auto& g : gates_) { g->Reset(); }
  for (auto& a : arcways_) { a->Reset(); }
}

HoldPoint Airport::GetHoldPoint(std::string take_off_runway) {
  for (auto hp : holdpoints_) {
    if (hp.hold_for_take_off_runway == take_off_runway && hp.type == HoldPointType::TAKEOFF) {
      return hp;
    }
  }
  return HoldPoint();
}

HoldPoint Airport::GetLineUpPoint(std::string take_off_runway) {
  for (auto lp : holdpoints_) {
    if (lp.hold_for_take_off_runway == take_off_runway && lp.type == HoldPointType::LINEUP) {
      return lp;
    }
  }
  return HoldPoint();
}

std::string Airport::GetRunwayInternalName(std::string calling_name) {
  return calling_name_to_internal_name_[calling_name];
}

LandingPositionInfo Airport::GetLandingPositionInfo(RouteBase* runway) {
  LandingPositionInfo info;
  info.runway = runway;
  info.direction = ! mode_;
  info.distance = mode_ ? 2300 : 400;
  return info;
}

void Airport::SetWindDirection(float wind_direction) {
  wind_direction_ = wind_direction;
}

float Airport::GetWindDirection() {
  return wind_direction_;
}

Airport::~Airport() {
}
