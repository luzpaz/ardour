/*
 * Copyright (C) 2009-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2009-2017 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2009 Hans Baier <hansfbaier@googlemail.com>
 * Copyright (C) 2012-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2012 Ben Loftis <ben@harrisonconsoles.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*****************************************************
    DO NOT USE uint8_t or any other type that resolves
    to a single char, because the value will be
    stored incorrectly when serialized. Use int32_t
    instead and ensure that code correctly limits
    the value of the variable.
*****************************************************/

CONFIG_VARIABLE (bool, use_region_fades, "use-region-fades", true)
CONFIG_VARIABLE (bool, use_transport_fades, "use-transport-fades", true)
CONFIG_VARIABLE (bool, use_monitor_fades, "use-monitor-fades", true)
CONFIG_VARIABLE (SampleFormat, native_file_data_format,  "native-file-data-format", ARDOUR::FormatFloat)
CONFIG_VARIABLE (HeaderFormat, native_file_header_format,  "native-file-header-format", ARDOUR::RF64_WAV)
CONFIG_VARIABLE (bool, auto_play, "auto-play", false)
CONFIG_VARIABLE (bool, auto_return, "auto-return", false)
CONFIG_VARIABLE (bool, auto_input, "auto-input", true)
CONFIG_VARIABLE (bool, triggerbox_overrides_disk_monitoring, "triggerbox-overrides-disk-monitoring", true)
CONFIG_VARIABLE (CueBehavior, cue_behavior, "cue-behavior", FollowCues)
CONFIG_VARIABLE (bool, punch_in, "punch-in", false)
CONFIG_VARIABLE (bool, punch_out, "punch-out", false)
CONFIG_VARIABLE (bool, count_in, "count-in", false)
CONFIG_VARIABLE (MonitorChoice, session_monitoring, "session-monitoring", MonitorAuto)
CONFIG_VARIABLE (RecordMode, record_mode, "record-mode", RecLayered)
CONFIG_VARIABLE (bool, draw_opaque_midi_regions, "draw-opaque-midi_regions", false)
CONFIG_VARIABLE (uint32_t, subframes_per_frame, "subframes-per-frame", 100)
CONFIG_VARIABLE (Timecode::TimecodeFormat, timecode_format, "timecode-format", Timecode::timecode_30)
CONFIG_VARIABLE (samplecnt_t, minitimeline_span, "minitimeline-span", 120) // seconds
CONFIG_VARIABLE_SPECIAL(std::string, raid_path, "raid-path", "", PBD::path_expand)
CONFIG_VARIABLE_SPECIAL(std::string, audio_search_path, "audio-search-path", "", PBD::search_path_expand)
CONFIG_VARIABLE_SPECIAL(std::string, midi_search_path, "midi-search-path", "", PBD::search_path_expand)
CONFIG_VARIABLE (bool, track_name_number, "track-name-number", false)
CONFIG_VARIABLE (bool, track_name_take, "track-name-take", true)
CONFIG_VARIABLE (std::string, take_name, "take-name", "Take1")
CONFIG_VARIABLE (bool, jack_time_master, "jack-time-master", true)
CONFIG_VARIABLE (bool, use_video_sync, "use-video-sync", false)
CONFIG_VARIABLE (float, video_pullup, "video-pullup", 0.0f)
CONFIG_VARIABLE (bool, external_sync, "external-sync", false)
CONFIG_VARIABLE (InsertMergePolicy, insert_merge_policy, "insert-merge-policy", InsertMergeRelax)
CONFIG_VARIABLE (samplecnt_t, timecode_offset, "timecode-offset", 0)
CONFIG_VARIABLE (bool, timecode_offset_negative, "timecode-offset-negative", true)
CONFIG_VARIABLE (std::string, slave_timecode_offset, "slave-timecode-offset", " 00:00:00:00")
CONFIG_VARIABLE (std::string, timecode_generator_offset, "timecode-generator-offset", " 00:00:00:00")
CONFIG_VARIABLE (bool, midi_copy_is_fork, "midi-copy-is-fork", true)
CONFIG_VARIABLE (bool, tracks_follow_session_time, "tracks-follow-session-time", false)
CONFIG_VARIABLE (bool, realtime_export, "realtime-export", false)
CONFIG_VARIABLE (bool, use_surround_master, "use-surround-master", false)

/* Video-settings are saved with the session and belong to the session.
 * headless ardour could remote control xjadeo for example.
 */
CONFIG_VARIABLE (bool, use_video_file_fps, "use-video-file-fps", false)
CONFIG_VARIABLE (bool, videotimeline_pullup, "videotimeline-pullup", true)

/* These are GUI-only properties and should not be present in this
 * context. There needs to be a new GUI-level session-scoped configuration
 * variable header.
 */
CONFIG_VARIABLE (double, wave_amplitude_zoom, "wave-amplitude-zoom", 0.0)
CONFIG_VARIABLE (uint16_t, wave_zoom_factor, "wave-zoom-factor", 2)
CONFIG_VARIABLE (bool, show_summary, "show-summary", true)
CONFIG_VARIABLE (bool, show_group_tabs, "show-group-tabs", true)
CONFIG_VARIABLE (bool, show_region_fades, "show-region-fades", true)
CONFIG_VARIABLE (bool, show_busses_on_meterbridge, "show-busses-on-meterbridge", false)
CONFIG_VARIABLE (bool, show_master_on_meterbridge, "show-master-on-meterbridge", true)
CONFIG_VARIABLE (bool, show_midi_on_meterbridge, "show-midi-on-meterbridge", true)
CONFIG_VARIABLE (bool, show_rec_on_meterbridge, "show-rec-on-meterbridge", true)
CONFIG_VARIABLE (bool, show_mute_on_meterbridge, "show-mute-on-meterbridge", false)
CONFIG_VARIABLE (bool, show_solo_on_meterbridge, "show-solo-on-meterbridge", false)
CONFIG_VARIABLE (bool, show_monitor_on_meterbridge, "show-monitor-on-meterbridge", false)
CONFIG_VARIABLE (bool, show_name_on_meterbridge, "show-name-on-meterbridge", true)
CONFIG_VARIABLE (bool, show_fader_on_meterbridge, "show-fader-on-meterbridge", false)
CONFIG_VARIABLE (uint32_t, meterbridge_label_height,  "meterbridge-label-height", 0)
CONFIG_VARIABLE (bool, show_master_bus_comment_on_load, "show-master-bus-comment-on-load", false)

/* If the user changes the session default_time_domain, we also stash that in rc_config as a global preference,
     where it is used to initialize the session timebase menu during new session creation
*/
CONFIG_VARIABLE (Temporal::TimeDomain, default_time_domain, "default-time-domain", Temporal::BeatTime)
