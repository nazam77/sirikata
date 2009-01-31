/*  Sirikata Transfer -- Content Transfer management system
 *  TransferData.hpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SIRIKATA_TransferData_HPP__
#define SIRIKATA_TransferData_HPP__

#include <boost/shared_ptr.hpp>
#include <vector>
#include <list>
#include <iostream>
#include "Range.hpp"

namespace Sirikata {
namespace Transfer {


/// Represents a single block of data, and also knows the range of the file it came from.
class DenseData : boost::noncopyable, public Range {
	std::vector<unsigned char> mData;

public:
	DenseData(const Range &range)
			:Range(range) {
		if (range.length()) {
			mData.resize(range.length());
		}
	}

	inline const unsigned char *data() const {
		return &(mData[0]);
	}

	inline unsigned char *writableData() {
		return &(mData[0]);
	}

	inline const unsigned char *dataAt(base_type offset) const {
		if (offset >= endbyte() || offset < startbyte()) {
			return NULL;
		}
		return &(mData[offset-startbyte()]);
	}

	inline void setLength(size_t len, bool is_npos) {
		Range::setLength(len, is_npos);
		mData.resize(len);
		//message1.reserve(size);
		//std::copy(data, data+len, std::back_inserter(mData));
	}
};

typedef boost::shared_ptr<DenseData> DenseDataPtr;

/// Represents a series of DenseData.  Often you may have adjacent DenseData.
class SparseData {
	typedef std::list<DenseDataPtr> ListType;

	///sorted vector of Range/vector pairs
	ListType mSparseData;
public:
	typedef DenseDataPtr value_type;

	/// Simple stub iterator class for use by Range::isContainedBy()
	class iterator : public ListType::iterator {
	public:
		iterator(const ListType::iterator &e) :
			ListType::iterator(e) {
		}
		inline DenseData &operator* () {
			return *(this->ListType::iterator::operator*());
		}

		inline const DenseDataPtr &getPtr() {
			return this->ListType::iterator::operator*();
		}
	};

	inline iterator begin() {
		return mSparseData.begin();
	}
	inline iterator end() {
		return mSparseData.end();
	}

	/// Simple stub iterator class for use by Range::isContainedBy()
	class const_iterator : public ListType::const_iterator {
	public:
		const_iterator(const ListType::const_iterator &e) :
			ListType::const_iterator(e) {
		}
		const_iterator(const ListType::iterator &e) :
			ListType::const_iterator(e) {
		}
		inline const DenseData &operator* () const {
			return *(this->ListType::const_iterator::operator*());
		}
	};
	inline const_iterator begin() const {
		return mSparseData.begin();
	}
	inline const_iterator end() const {
		return mSparseData.end();
	}

	inline iterator insert(const iterator &iter, const value_type &dd) {
		return mSparseData.insert(iter, dd);
	}

	inline void erase(const iterator &iter) {
		mSparseData.erase(iter);
	}

	inline void clear() {
		return mSparseData.clear();
	}

	///adds a range of valid data to the SparseData set.
	void addValidData(const DenseDataPtr &data) {
		data->addToList(data, *this);
	}

	///gets the space used by the sparse file
	inline cache_usize_type getSpaceUsed() const {
		cache_usize_type length = 0;
		const_iterator myend = end();
		for (const_iterator iter = begin(); iter != myend; ++iter) {
			length += (*iter).length();
		}
		return length;
	}

	void debugPrint(std::ostream &os) const {
		Range::base_type position = 0, len;
		do {
			const unsigned char *data = dataAt(position, len);
			if (data) {
				std::cout << "{GOT DATA "<<len<<"}";
				std::cout << std::string(data, data+len);
			} else if (len) {
				std::cout << "[INVALID:" <<len<< "]";
			}
			position += len;
		} while (len);
		std::cout << std::endl;
	}

	/**
	 * gets a pointer to data starting at offset
	 * @param offset specifies where in the sparse file the data should be gotten from
	 * @param length returns for how much the data is valid
	 * @returns data at that point unless offset is not yet valid in which case dataAt returns NULL with
	 *          length being the range of INVALID data, or 0 if there is not valid data past that point
	 */
	const unsigned char *dataAt(Range::base_type offset, Range::length_type &length) const {
		const_iterator enditer = end();
		for (const_iterator iter = begin(); iter != enditer; ++iter) {
			const Range &range = (*iter);
			if (offset >= range.startbyte() &&
					(range.goesToEndOfFile() || offset < range.endbyte())) {
				// We're within some valid data... return the DenseData.
				length = range.length() + (Range::length_type)(range.startbyte() - offset);
				return (*iter).dataAt(offset);
			} else if (offset < range.startbyte()){
				// we missed it.
				length = (size_t)(range.startbyte() - offset);
				return NULL;
			}
		}
		length = 0;
		return NULL;
	}

};
//typedef boost::shared_ptr<SparseData> SparseDataPtr;

}
}

#endif /* SIRIKATA_TransferData_HPP__ */