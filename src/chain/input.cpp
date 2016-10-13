/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/bitcoin/chain/input.hpp>

#include <sstream>
#include <boost/iostreams/stream.hpp>
#include <bitcoin/bitcoin/constants.hpp>
#include <bitcoin/bitcoin/utility/container_sink.hpp>
#include <bitcoin/bitcoin/utility/container_source.hpp>
#include <bitcoin/bitcoin/utility/istream_reader.hpp>
#include <bitcoin/bitcoin/utility/ostream_writer.hpp>

namespace libbitcoin {
namespace chain {

input input::factory_from_data(const data_chunk& data)
{
    input instance;
    instance.from_data(data);
    return instance;
}

input input::factory_from_data(std::istream& stream)
{
    input instance;
    instance.from_data(stream);
    return instance;
}

input input::factory_from_data(reader& source)
{
    input instance;
    instance.from_data(source);
    return instance;
}

input::input()
  : previous_output_(), script_(), sequence_(0)
{
}

input::input(const output_point& previous_output, const chain::script& script,
    uint32_t sequence)
  : previous_output_(previous_output), script_(script), sequence_(sequence)
{
}

input::input(output_point&& previous_output, chain::script&& script,
    uint32_t sequence)
  : previous_output_(std::move(previous_output)), script_(std::move(script)),
    sequence_(sequence)
{
}

input::input(const input& other)
  : input(other.previous_output_, other.script_, other.sequence_)
{
}

input::input(input&& other)
  : input(std::move(other.previous_output_), std::move(other.script_),
      other.sequence_)
{
}

// Since empty script and zero sequence are valid this relies othe prevout.
bool input::is_valid() const
{
    return (sequence_ != 0) || previous_output_.is_valid()
        || script_.is_valid();
}

void input::reset()
{
    previous_output_.reset();
    script_.reset();
    sequence_ = 0;
}

bool input::from_data(const data_chunk& data)
{
    data_source istream(data);
    return from_data(istream);
}

bool input::from_data(std::istream& stream)
{
    istream_reader source(stream);
    return from_data(source);
}

bool input::from_data(reader& source)
{
    reset();

    auto result = previous_output_.from_data(source);

    if (result)
    {
        // Parse the coinbase tx as raw data.
        // Always parse non-coinbase input/output scripts as fallback.
        const auto mode = previous_output_.is_null() ?
            script::parse_mode::raw_data : 
            script::parse_mode::raw_data_fallback;

        result = script_.from_data(source, true, mode);
    }

    if (result)
    {
        sequence_ = source.read_4_bytes_little_endian();
        result = source;
    }

    if (!result)
        reset();

    return result;
}

data_chunk input::to_data() const
{
    data_chunk data;
    data_sink ostream(data);
    to_data(ostream);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == serialized_size());
    return data;
}

void input::to_data(std::ostream& stream) const
{
    ostream_writer sink(stream);
    to_data(sink);
}

void input::to_data(writer& sink) const
{
    previous_output_.to_data(sink);
    script_.to_data(sink, true);
    sink.write_4_bytes_little_endian(sequence_);
}

uint64_t input::serialized_size() const
{
    const auto script_size = script_.serialized_size(true);
    return 4 + previous_output_.serialized_size() + script_size;
}

size_t input::signature_operations(bool bip16_active) const
{
    auto sigops = script_.sigops(false);

    if (bip16_active)
    {
        // This cannot overflow because each total is limited by max ops.
        const auto& cache = previous_output_.validation.cache.script();
        sigops += script_.pay_script_hash_sigops(cache);
    }

    return sigops;
}

std::string input::to_string(uint32_t flags) const
{
    std::ostringstream ss;

    ss << previous_output_.to_string() << "\n"
        << "\t" << script_.to_string(flags) << "\n"
        << "\tsequence = " << sequence_ << "\n";

    return ss.str();
}

bool input::is_final() const
{
    return (sequence_ == max_input_sequence);
}

output_point& input::previous_output()
{
    return previous_output_;
}

const output_point& input::previous_output() const
{
    return previous_output_;
}

void input::set_previous_output(const output_point& value)
{
    previous_output_ = value;
}

void input::set_previous_output(output_point&& value)
{
    previous_output_ = std::move(value);
}

chain::script& input::script()
{
    return script_;
}

const chain::script& input::script() const
{
    return script_;
}

void input::set_script(const chain::script& value)
{
    script_ = value;
}

void input::set_script(chain::script&& value)
{
    script_ = std::move(value);
}

uint32_t input::sequence() const
{
    return sequence_;
}

void input::set_sequence(uint32_t value)
{
    sequence_ = value;
}

input& input::operator=(const input& other)
{
    previous_output_ = other.previous_output_;
    script_ = other.script_;
    sequence_ = other.sequence_;
    return *this;
}

input& input::operator=(input&& other)
{
    previous_output_ = std::move(other.previous_output_);
    script_ = std::move(other.script_);
    sequence_ = other.sequence_;
    return *this;
}

bool input::operator==(const input& other) const
{
    return (sequence_ == other.sequence_)
        && (previous_output_ == other.previous_output_)
        && (script_ == other.script_);
}

bool input::operator!=(const input& other) const
{
    return !(*this == other);
}

} // namespace chain
} // namespace libbitcoin