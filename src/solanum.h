#pragma once

#include <time.h>

typedef int32_t bool32;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

#pragma pack(push)
#pragma pack(1)
struct TimeRecord
{
	int16 elapsed;
    int64 timestamp;
};
#pragma pack(pop)

struct TimerState
{
    int window_width; // This refers to the main imgui window
    int window_height;

    TimeRecord* records;
    size_t records_size;
    int64 num_records;

    int64 time_unit_in_s;
    int64 num_seconds;

    time_t begin_time;
    bool32 started;
    bool32 is_long;

    time_t pause_time;
    int16 pause_elapsed;
    bool32 paused;

    int64 time_persp;  // Point of reference for timer quick report
};

static char* hilarious_phrases[] =
{
    "Get some shit done!",
};

#define TEXT_BUFFER_SIZE 256
static void format_seconds(char* buffer, char* msg, int64 in_seconds)
{
    int64 hours = in_seconds / (60 * 60);
    int64 minutes = (in_seconds / 60) % 60;
    int64 seconds = in_seconds % 60;
    snprintf(buffer, TEXT_BUFFER_SIZE, "%s: %uh %um %us", msg, hours, minutes, seconds);
}

static void timer_step_and_render(TimerState* state)
{
    char buffer[TEXT_BUFFER_SIZE];
    time_t current_time;
    time(&current_time);

    ImVec2 size = {(float)400, (float)200};
    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.17f, 0.23f, 0.42f, 0.5});

    ImGui::Begin("Solanum", NULL, size);
    ImGui::SetWindowSize(size);

    if (!state->started)
    {
        ImGui::Text("Perspective: ");
        ImGui::SameLine();
        bool change_persp = false;
        if (ImGui::Button("Now"))
        {
            change_persp = true;
            state->time_persp = current_time;
        }
        ImGui::SameLine();
        if (ImGui::Button("-1 hour"))
        {
            change_persp = true;
            state->time_persp -= 1 * 60 * 60;
        }
        ImGui::SameLine();
        if (ImGui::Button("-8 hours"))
        {
            change_persp = true;
            state->time_persp -= 8 * 60 * 60;
        }
        ImGui::SameLine();
        if (ImGui::Button("-24 hours"))
        {
            change_persp = true;
            state->time_persp -= 24 * 60 * 60;
        }

        if (change_persp)
        {
            state->num_seconds = 0;
            for (int i = 0; i < state->num_records; ++i)
            {
                if (state->records[i].timestamp >= state->time_persp)
                {
                    state->num_seconds += state->records[i].elapsed;
                }
            }
        }

        ImGui::Separator();
        format_seconds(buffer, "Time logged", state->num_seconds);
        ImGui::Text(buffer);
        ImGui::Spacing();
        state->is_long = false;
        if (ImGui::Button("Begin (short)"))
        {
            state->started = true;
            time_t begin_time;
            time(&begin_time);
            state->begin_time = begin_time;
        }
        ImGui::SameLine(0, 30);
        if (ImGui::Button("Begin (long)"))
        {
            state->started = true;
            time_t begin_time;
            time(&begin_time);
            state->begin_time = begin_time;
            state->is_long = true;
        }

        ImGui::Separator();

        ImGui::SetCursorPosY(size.y - 80);
        if (ImGui::Button("Quit"))
        {
            platform_quit();
        }
    }

    if (state->started)
    {
        int16 elapsed = 0;
        {
            int64 elapsed_64 = current_time - state->begin_time - state->pause_elapsed;
            if (elapsed_64 <= (1 << 15))
            {
                elapsed = (int16) elapsed_64;
            }
            else
            {
                state->started = false;
            }
        }

        //elapsed += 29 * 60 + 0;
        int16 factor = state->is_long? 2 : 1;
        int64 time_left = (state->time_unit_in_s * factor) - elapsed;

        format_seconds(buffer, "Time elapsed", elapsed);
        ImGui::Text(buffer);

        if (!state->paused)
        {
            if (ImGui::Button("Pause"))
            {
                state->pause_time = current_time - state->pause_elapsed;
                state->paused = true;
            }
        }
        else
        {
            state->pause_elapsed = (int16)(current_time - state->pause_time);
            if (ImGui::Button("Resume"))
            {
                state->paused = false;
            }
        }

        ImGui::SameLine(0, 30);
        bool32 stopping = ImGui::Button("Stop");

        bool32 alert_user = false;
        if (time_left <= 0)
        {
            stopping = true;
            alert_user = true;
        }

        int num_phrases = (int)sizeof(hilarious_phrases) / sizeof(char*);
        char* phrase = hilarious_phrases[current_time % num_phrases];
        ImGui::Separator();
        ImGui::Text(phrase);

        if (stopping)
        {
            state->started = false;
            state->pause_elapsed = 0;
            state->paused = false;
            TimeRecord record = {};
            record.timestamp = current_time;

            // This can happen if we leave a timer running and the computer goes to sleep.
            if (elapsed > (int16)state->time_unit_in_s * factor)
            {
                elapsed = (int16)state->time_unit_in_s * factor;
            }

            record.elapsed = elapsed;
            state->num_seconds += elapsed;
            state->records[state->num_records++] = record;
            if (alert_user)
            {
                platform_alert();
            }
            platform_save_state(state);
        }
    }

    ImGui::End();
    ImGui::Render();
}
