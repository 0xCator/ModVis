#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>


#define BAR_WIDTH 40
#define BAR_SPACING 20
#define CHART_HEIGHT 500
#define SCREEN_WIDTH 1400
#define SCREEN_HEIGHT 720

#define da_append(v, e) do { \
    if ((v).size == (v).capacity) { \
        (v).capacity = (v).capacity == 0 ? 1 : (v).capacity * 2; \
        (v).reports = realloc((v).reports, sizeof(SmellReport) * (v).capacity); \
        if (!(v).reports) { \
            perror("Realloc failed"); \
            exit(EXIT_FAILURE); \
        } \
    } \
    (v).reports[(v).size++] = e; \
} while (0)

typedef enum {
    GODCLASS,
    DATACLASS,
    FEATUREENVY,
    LONGMETHOD,
    LONGPARAMETERLIST,
} Smell;

typedef struct {
    size_t fp;
    size_t fn;
    size_t tp;
    size_t tn;
    float accuracy;
    float precision;
    float recall;
    float f1;
    size_t fp0;
    size_t fn0;
    size_t tp0;
    size_t tn0;
    float accuracy0;
    float precision0;
    float recall0;
    float f10;
    Smell smell;
} SmellReport;

typedef struct {
    char name[256];
    SmellReport* reports;
    size_t size;
    size_t capacity;
} ModelReport;

typedef struct {
    ModelReport* reports;
    size_t size;
    size_t capacity;
} Report;

static Report report = {0};
static Smell selected_smell = GODCLASS;

static void eval(SmellReport* report) {
    report->accuracy = (float)(report->tp + report->tn) /
                       (report->tp + report->tn + report->fp + report->fn);
    report->precision = report->tp ?
                        (float)report->tp / (report->tp + report->fp) : 0.0f;
    report->recall = report->tp ?
                     (float)report->tp / (report->tp + report->fn) : 0.0f;
    report->f1 = (report->precision + report->recall) > 0 ?
                 2 * (report->precision * report->recall) /
                 (report->precision + report->recall) : 0.0f;
    report->accuracy0 = (float)(report->tp0 + report->tn0) /
                        (report->tp0 + report->tn0 + report->fp0 + report->fn0);
    report->precision0 = report->tp0 ?
                         (float)report->tp0 / (report->tp0 + report->fp0) : 0.0f;
    report->recall0 = report->tp0 ?
                      (float)report->tp0 / (report->tp0 + report->fn0) : 0.0f;
    report->f10 = (report->precision0 + report->recall0) > 0 ?
                  2 * (report->precision0 * report->recall0) /
                  (report->precision0 + report->recall0) : 0.0f;

}

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

static void add_models() {
    FILE* file = fopen("inputs.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < 5; i++) {
        ModelReport* model_report = (ModelReport*)malloc(sizeof(ModelReport));
        if (!model_report) {
            perror("Malloc failed");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        memset(model_report, 0, sizeof(ModelReport));

        char line[256];
        char c;

        while (is_space(c = fgetc(file)));
        ungetc(c, file);

        if (fgets(line, sizeof(line), file) != NULL) {
            sscanf(line, "%255s", model_report->name);
        }

        for (size_t j = 0; j < 5; j++) {
            SmellReport smell_report = {0};
            smell_report.smell = j;

            fscanf(file, "%lu %lu %lu %lu", &smell_report.tn, &smell_report.fn, &smell_report.fp, &smell_report.tp);
            smell_report.tn0 = smell_report.tp;
            smell_report.fn0 = smell_report.fp;
            smell_report.fp0 = smell_report.fn;
            smell_report.tp0 = smell_report.tn;


            eval(&smell_report);
            da_append(*model_report, smell_report);
        }

        if (report.size == report.capacity) {
            report.capacity = report.capacity == 0 ? 1 : report.capacity * 2;
            report.reports = realloc(report.reports, sizeof(ModelReport) * report.capacity);
            if (!report.reports) {
                perror("Realloc failed");
                fclose(file);
                exit(EXIT_FAILURE);
            }
        }

        da_append(report, *model_report);
    }

    fclose(file);
}

static int screenshot_mode = 0;
static int zero_mode = 0;

static void render_chart() {
    const Color metricColors[] = { RED, GREEN, BLUE, ORANGE }; // Colors for metrics
    const char* metricLabels[] = { "Recall", "Precision", "F1", "Accuracy" };

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Models Visualization");
    Font customFont = LoadFontEx("arial.ttf", 20, NULL, 0); // Custom font
    if (customFont.texture.id == 0) {
        printf("Error: Failed to load font.\n");
        CloseWindow();
        return;
    }

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        const int chartX = 100, chartY = 50;
        const int legendWidth = 150;
        const int chartWidth = SCREEN_WIDTH - chartX - legendWidth - 20;
        const int chartHeight = CHART_HEIGHT;

        DrawLine(chartX, chartY + chartHeight, chartX + chartWidth, chartY + chartHeight, BLACK); // X-axis
        DrawLine(chartX, chartY, chartX, chartY + chartHeight, BLACK);                           // Y-axis

        for (int i = 1; i <= 10; i++) {
            int y = chartY + chartHeight - (i * chartHeight / 10);
            DrawLine(chartX, y, chartX + chartWidth, y, LIGHTGRAY); // Horizontal gridline
            DrawTextEx(customFont, TextFormat("%.1f", (float)i / 10), (Vector2){ chartX - 30, y - 10 }, 20, 2, GRAY); // Y-axis labels
        }

        int numModels = report.size;
        int barGroupWidth = (chartWidth - (numModels + 1) * BAR_SPACING) / numModels;
        int barWidth = barGroupWidth / 4; 

        for (int i = 0; i < numModels; i++) {
            ModelReport model = report.reports[i];
            int groupX = chartX + BAR_SPACING + i * (barGroupWidth + BAR_SPACING);

            // Draw bars for each metric for the selected smell
            SmellReport* smell_report = NULL;
            for (size_t j = 0; j < model.size; j++) {
                if (model.reports[j].smell == selected_smell) {
                    smell_report = &model.reports[j];
                    break;
                }
            }
            if(IsKeyPressed(KEY_SPACE)){
                zero_mode = !zero_mode;
            }

            if (smell_report) {
                int groupX = chartX + BAR_SPACING + i * (barGroupWidth + BAR_SPACING);

                // Draw bars for each metric
                for (int j = 0; j < 4; j++) {
                    float value = 0.0f;
                    if (!zero_mode) {
                        switch (j) {
                            case 0: value = smell_report->recall; break;
                            case 1: value = smell_report->precision; break;
                            case 2: value = smell_report->f1; break;
                            case 3: value = smell_report->accuracy; break;
                        }
                    } else{
                        switch (j) {
                            case 0: value = smell_report->precision0; break;
                            case 1: value = smell_report->recall0; break;
                            case 2: value = smell_report->f10; break;
                            case 3: value = smell_report->accuracy0; break;
                        }
                    }

                    int barHeight = (int)(value * chartHeight);
                    int barX = groupX + j * barWidth;
                    int barY = chartY + chartHeight - barHeight;

                    DrawRectangle(barX, barY, barWidth, barHeight, metricColors[j]);
                    DrawTextEx(customFont, TextFormat("%.3f", value), (Vector2){ barX, barY - 20 }, 20, 1, DARKGRAY);
                }
            }

            // Model name below the bars
            DrawTextEx(customFont, model.name, (Vector2){ groupX, chartY + chartHeight + 10 }, 20, 2, DARKGRAY);
        }

        // Metric legends
        int legendX = chartX + chartWidth + 20;
        int legendY = chartY;

        for (int i = 0; i < 4; i++) {
            DrawRectangle(legendX, legendY + i * 30, 20, 20, metricColors[i]); // Legend color box
            DrawTextEx(customFont, metricLabels[i], (Vector2){ legendX + 30, legendY + i * 30 }, 20, 2, BLACK); // Metric name
        }

        if (IsKeyPressed(KEY_R)) {
            screenshot_mode = !screenshot_mode;
        }

        if(!screenshot_mode) {
            // Draw buttons for each smell
            int buttonWidth = 240;
            int buttonHeight = 40;
            int buttonY = chartY + chartHeight + 50;

            for (int i = 0; i < 5; i++) {
                int buttonX = chartX + i * (buttonWidth + 10);
                Rectangle buttonRect = { buttonX, buttonY, buttonWidth, buttonHeight };
                Color buttonColor = ((int)selected_smell == i) ? LIGHTGRAY : GRAY;

                if (CheckCollisionPointRec(GetMousePosition(), buttonRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    selected_smell = i;
                }

                DrawRectangleRec(buttonRect, buttonColor);
                DrawTextEx(customFont, TextFormat("%s", (i == 0 ? "GODCLASS" : i == 1 ? "DATACLASS" : i == 2 ? "FEATUREENVY" : i == 3 ? "LONGMETHOD" : "LONGPARAMETERLIST")), (Vector2){ buttonX + 10, buttonY + 10 }, 20, 2, BLACK);
            }
        } 
        DrawTextEx(customFont, TextFormat("%s", (selected_smell == 0 ? "GODCLASS" : selected_smell == 1 ? "DATACLASS" : selected_smell == 2 ? "FEATUREENVY" : selected_smell == 3 ? "LONGMETHOD" : "LONGPARAMETERLIST")), (Vector2){ 10, 10 }, 20, 2, BLACK);
        DrawTextEx(customFont, TextFormat("Class: %s", zero_mode ? "0" : "1"), (Vector2){ 250, 10 }, 20, 2, BLACK);


        EndDrawing();
        if (IsKeyPressed(KEY_S)) {
            char* smell = (selected_smell == 0 ? "GODCLASS" : selected_smell == 1 ? "DATACLASS" : selected_smell == 2 ? "FEATUREENVY" : selected_smell == 3 ? "LONGMETHOD" : "LONGPARAMETERLIST");
            char* zero = zero_mode ? "0" : "1";
            TakeScreenshot(TextFormat("chart_%s_%s.png", smell, zero));
        }
    }

    // Clean up
    UnloadFont(customFont);
    CloseWindow();
}

int main() {
    add_models();
    render_chart();
    return EXIT_SUCCESS;
}

