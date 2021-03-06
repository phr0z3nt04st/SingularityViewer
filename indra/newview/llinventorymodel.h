/** 
 * @file llinventorymodel.h
 * @brief LLInventoryModel class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLINVENTORYMODEL_H
#define LL_LLINVENTORYMODEL_H

#include "llassettype.h"
#include "llfoldertype.h"
#include "lldarray.h"
#include "llhttpclient.h"
#include "lluuid.h"
#include "llpermissionsflags.h"
#include "llstring.h"

#include "llmd5.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class LLInventoryObserver;

class LLInventoryObject;
class LLInventoryItem;
class LLInventoryCategory;
class LLViewerInventoryItem;
class LLViewerInventoryCategory;
class LLViewerInventoryItem;
class LLViewerInventoryCategory;
class LLMessageSystem;
class LLInventoryCollectFunctor;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLInventoryModel
//
// Represents a collection of inventory, and provides efficient ways to access 
// that information.
//   NOTE: This class could in theory be used for any place where you need 
//   inventory, though it optimizes for time efficiency - not space efficiency, 
//   probably making it inappropriate for use on tasks.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryModel
{
public:
	enum EHasChildren
	{
		CHILDREN_NO,
		CHILDREN_YES,
		CHILDREN_MAYBE
	};

	typedef LLDynamicArray<LLPointer<LLViewerInventoryCategory> > cat_array_t;
	typedef LLDynamicArray<LLPointer<LLViewerInventoryItem> > item_array_t;
	typedef std::set<LLUUID> changed_items_t;

	class fetchInventoryResponder : public LLHTTPClient::Responder
	{
	public:
		fetchInventoryResponder(const LLSD& request_sd) : mRequestSD(request_sd) {};
		void result(const LLSD& content);			
		void error(U32 status, const std::string& reason);
	public:
		typedef std::vector<LLViewerInventoryCategory*> folder_ref_t;
	protected:
		LLSD mRequestSD;
	};

/********************************************************************************
 **                                                                            **
 **                    INITIALIZATION/SETUP
 **/

	//--------------------------------------------------------------------
	// Constructors / Destructors
	//--------------------------------------------------------------------
public:
	LLInventoryModel();
	~LLInventoryModel();
	void cleanupInventory();
//<edit>
//protected:
//</edit>
	void empty(); // empty the entire contents

	//--------------------------------------------------------------------
	// Initialization
	//--------------------------------------------------------------------
public:
	// The inventory model usage is sensitive to the initial construction of the model
	bool isInventoryUsable() const;
private:
	bool mIsAgentInvUsable; // used to handle an invalid inventory state

	//--------------------------------------------------------------------
	// Root Folders
	//--------------------------------------------------------------------
public:
	// The following are set during login with data from the server
	void setRootFolderID(const LLUUID& id);
	void setLibraryOwnerID(const LLUUID& id);
	void setLibraryRootFolderID(const LLUUID& id);

	const LLUUID &getRootFolderID() const;
	const LLUUID &getLibraryOwnerID() const;
	const LLUUID &getLibraryRootFolderID() const;
private:
	LLUUID mRootFolderID;
	LLUUID mLibraryRootFolderID;
	LLUUID mLibraryOwnerID;	
	
	//--------------------------------------------------------------------
	// Structure
	//--------------------------------------------------------------------
public:
	// Methods to load up inventory skeleton & meat. These are used
	// during authentication. Returns true if everything parsed.
	bool loadSkeleton(const LLSD& options, const LLUUID& owner_id);
	void buildParentChildMap(); // brute force method to rebuild the entire parent-child relations
	// Call on logout to save a terse representation.
	void cache(const LLUUID& parent_folder_id, const LLUUID& agent_id);
private:
	// Information for tracking the actual inventory. We index this
	// information in a lot of different ways so we can access
	// the inventory using several different identifiers.
	// mInventory member data is the 'master' list of inventory, and
	// mCategoryMap and mItemMap store uuid->object mappings. 
	typedef std::map<LLUUID, LLPointer<LLViewerInventoryCategory> > cat_map_t;
	typedef std::map<LLUUID, LLPointer<LLViewerInventoryItem> > item_map_t;
	cat_map_t mCategoryMap;
	item_map_t mItemMap;
	// This last set of indices is used to map parents to children.
	typedef std::map<LLUUID, cat_array_t*> parent_cat_map_t;
	typedef std::map<LLUUID, item_array_t*> parent_item_map_t;
	parent_cat_map_t mParentChildCategoryTree;
	parent_item_map_t mParentChildItemTree;

	//--------------------------------------------------------------------
	// Login
	//--------------------------------------------------------------------
	const static S32 sCurrentInvCacheVersion; // expected inventory cache version

/**                    Initialization/Setup
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    ACCESSORS
 **/

	//--------------------------------------------------------------------
	// Descendents
	//--------------------------------------------------------------------
public:
	// Make sure we have the descendents in the structure.  Returns true
	// if a fetch was performed.
	bool fetchDescendentsOf(const LLUUID& folder_id) const;

	// Return the direct descendents of the id provided.Set passed
	// in values to NULL if the call fails.
	//    NOTE: The array provided points straight into the guts of
	//    this object, and should only be used for read operations, since
	//    modifications may invalidate the internal state of the inventory.
	void getDirectDescendentsOf(const LLUUID& cat_id,
								cat_array_t*& categories,
								item_array_t*& items) const;
	// Same but only get categories.
	void getDirectDescendentsOf(const LLUUID& cat_id,
								cat_array_t*& categories) const;
	
	// Compute a hash of direct descendent names (for detecting child name changes)
	LLMD5 hashDirectDescendentNames(const LLUUID& cat_id) const;

	// Starting with the object specified, add its descendents to the
	// array provided, but do not add the inventory object specified
	// by id. There is no guaranteed order. 
	//    NOTE: Neither array will be erased before adding objects to it. 
	//    Do not store a copy of the pointers collected - use them, and 
	//    collect them again later if you need to reference the same objects.
	enum { 
		EXCLUDE_TRASH = FALSE, 
		INCLUDE_TRASH = TRUE 
	};
	void collectDescendents(const LLUUID& id,
							cat_array_t& categories,
							item_array_t& items,
							BOOL include_trash);
	void collectDescendentsIf(const LLUUID& id,
							  cat_array_t& categories,
							  item_array_t& items,
							  BOOL include_trash,
							  LLInventoryCollectFunctor& add,
							  BOOL follow_folder_links = FALSE);

	// Collect all items in inventory that are linked to item_id.
	// Assumes item_id is itself not a linked item.
	item_array_t collectLinkedItems(const LLUUID& item_id,
									const LLUUID& start_folder_id = LLUUID::null);
	

	// Check if one object has a parent chain up to the category specified by UUID.
	BOOL isObjectDescendentOf(const LLUUID& obj_id, const LLUUID& cat_id, const BOOL break_on_recursion=FALSE) const;

	//--------------------------------------------------------------------
	// Find
	//--------------------------------------------------------------------
public:
	// Returns the uuid of the category that specifies 'type' as what it 
	// defaults to containing. The category is not necessarily only for that type. 
	//    NOTE: If create_folder is true, this will create a new inventory category 
	//    on the fly if one does not exist. *NOTE: if find_in_library is true it 
	//    will search in the user's library folder instead of "My Inventory"
	const LLUUID findCategoryUUIDForType(LLFolderType::EType preferred_type, 
										 bool create_folder = true, 
										 bool find_in_library = false);
	
	// Get whatever special folder this object is a child of, if any.
	const LLViewerInventoryCategory *getFirstNondefaultParent(const LLUUID& obj_id) const;

	// Get the object by id. Returns NULL if not found.
	//   NOTE: Use the pointer returned for read operations - do
	//   not modify the object values in place or you will break stuff.
	LLInventoryObject* getObject(const LLUUID& id) const;

	// Get the item by id. Returns NULL if not found.
	//    NOTE: Use the pointer for read operations - use the
	//    updateItem() method to actually modify values.
	LLViewerInventoryItem* getItem(const LLUUID& id) const;

	// Get the category by id. Returns NULL if not found.
	//    NOTE: Use the pointer for read operations - use the
	//    updateCategory() method to actually modify values.
	LLViewerInventoryCategory* getCategory(const LLUUID& id) const;

	// Get the inventoryID or item that this item points to, else just return object_id
	const LLUUID& getLinkedItemID(const LLUUID& object_id) const;
	LLViewerInventoryItem* getLinkedItem(const LLUUID& object_id) const;
	
	LLUUID findCategoryByName(std::string name);
private:
	mutable LLPointer<LLViewerInventoryItem> mLastItem; // cache recent lookups	

	//--------------------------------------------------------------------
	// Count
	//--------------------------------------------------------------------
public:
	// Return the number of items or categories
	S32 getItemCount() const;
	S32 getCategoryCount() const;

/**                    Accessors
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    MUTATORS
 **/

public:
	// Change an existing item with a matching item_id or add the item
	// to the current inventory. Returns the change mask generated by
	// the update. No notification will be sent to observers. This
	// method will only generate network traffic if the item had to be
	// reparented.
	//    NOTE: In usage, you will want to perform cache accounting
	//    operations in LLInventoryModel::accountForUpdate() or
	//    LLViewerInventoryItem::updateServer() before calling this method.
	U32 updateItem(const LLViewerInventoryItem* item);

	// Change an existing item with the matching id or add
	// the category. No notifcation will be sent to observers. This
	// method will only generate network traffic if the item had to be
	// reparented.
	//    NOTE: In usage, you will want to perform cache accounting
	//    operations in accountForUpdate() or LLViewerInventoryCategory::
	//    updateServer() before calling this method.
	void updateCategory(const LLViewerInventoryCategory* cat);

	// Move the specified object id to the specified category and
	// update the internal structures. No cache accounting,
	// observer notification, or server update is performed.
	void moveObject(const LLUUID& object_id, const LLUUID& cat_id);

	//--------------------------------------------------------------------
	// Delete
	//--------------------------------------------------------------------
public:
	// Delete a particular inventory object by ID. Will purge one
	// object from the internal data structures, maintaining a
	// consistent internal state. No cache accounting, observer
	// notification, or server update is performed.
	void deleteObject(const LLUUID& id);
	void removeItem(const LLUUID& item_id);
	
	// Delete a particular inventory object by ID, and delete it from
	// the server. Also updates linked items.
	void purgeObject(const LLUUID& id);

	// Collects and purges the descendants of the id
	// provided. If the category is not found, no action is
	// taken. This method goes through the long winded process of
	// removing server representation of folders and items while doing
	// cache accounting in a fairly efficient manner. This method does
	// not notify observers (though maybe it should...)
	void purgeDescendentsOf(const LLUUID& id);

	// This method optimally removes the referenced categories and
	// items from the current agent's inventory in the database. It
	// performs all of the during deletion. The local representation
	// is not removed.
	void deleteFromServer(LLDynamicArray<LLUUID>& category_ids,
						  LLDynamicArray<LLUUID>& item_ids);

	//
	// Misc Methods 
	//


	// This method to prepares a set of mock inventory which provides
	// minimal functionality before the actual arrival of inventory.
	//void mock(const LLUUID& root_id);

	

	// call this method to request the inventory.
	//void requestFromServer(const LLUUID& agent_id);


	//--------------------------------------------------------------------
	// Creation
	//--------------------------------------------------------------------
public:
	// Returns the UUID of the new category. If you want to use the default 
	// name based on type, pass in a NULL to the 'name' parameter.
	LLUUID createNewCategory(const LLUUID& parent_id,
							 LLFolderType::EType preferred_type,
							 const std::string& name,
							 void (*callback)(const LLSD&, void*) = NULL,
							 void* user_data = NULL);
public:
	// Internal methods that add inventory and make sure that all of
	// the internal data structures are consistent. These methods
	// should be passed pointers of newly created objects, and the
	// instance will take over the memory management from there.
	void addCategory(LLViewerInventoryCategory* category);
	void addItem(LLViewerInventoryItem* item);

	// methods to load up inventory skeleton & meat. These are used
	// during authentication. return true if everything parsed.
	typedef std::map<std::string, std::string> response_t;
	typedef std::vector<response_t> options_t;


	//OGPX really screwed with the login process. This is needed until it's all sorted out.
	bool loadSkeleton(const options_t& options, const LLUUID& owner_id);
/**                    Mutators
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    CATEGORY ACCOUNTING
 **/

public:
	// Represents the number of items added or removed from a category.
	struct LLCategoryUpdate
	{
		LLCategoryUpdate() : mDescendentDelta(0) {}
		LLCategoryUpdate(const LLUUID& category_id, S32 delta) :
			mCategoryID(category_id),
			mDescendentDelta(delta) {}
		LLUUID mCategoryID;
		S32 mDescendentDelta;
	};
	typedef std::vector<LLCategoryUpdate> update_list_t;

	// This exists to make it easier to account for deltas in a map.
	struct LLInitializedS32
	{
		LLInitializedS32() : mValue(0) {}
		LLInitializedS32(S32 value) : mValue(value) {}
		S32 mValue;
		LLInitializedS32& operator++() { ++mValue; return *this; }
		LLInitializedS32& operator--() { --mValue; return *this; }
	};
	typedef std::map<LLUUID, LLInitializedS32> update_map_t;

	// Call when there are category updates.  Call them *before* the 
	// actual update so the method can do descendent accounting correctly.
	void accountForUpdate(const LLCategoryUpdate& update) const;
	void accountForUpdate(const update_list_t& updates);
	void accountForUpdate(const update_map_t& updates);

	// Return (yes/no/maybe) child status of category children.
	EHasChildren categoryHasChildren(const LLUUID& cat_id) const;

	// Returns true iff category version is known and theoretical
	// descendents == actual descendents.
	bool isCategoryComplete(const LLUUID& cat_id) const;
	
/**                    Category Accounting
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    NOTIFICATIONS
 **/

public:
	// Called by the idle loop.  Only updates if new state is detected.  Call 
	// notifyObservers() manually to update regardless of whether state change 
	// has been indicated.
	void idleNotifyObservers();

	// Call to explicitly update everyone on a new state.
	void notifyObservers();
	// Allows outsiders to tell the inventory if something has
	// been changed 'under the hood', but outside the control of the
	// inventory. The next notify will include that notification.
	void addChangedMask(U32 mask, const LLUUID& referent);
	const changed_items_t& getChangedIDs() const { return mChangedItemIDs; }
protected:
	// Updates all linked items pointing to this id.
	void addChangedMaskForLinks(const LLUUID& object_id, U32 mask);
private:
	// Flag set when notifyObservers is being called, to look for bugs
	// where it's called recursively.
	BOOL mIsNotifyObservers;
	// Variables used to track what has changed since the last notify.
	U32 mModifyMask;
	changed_items_t mChangedItemIDs;
	
	//--------------------------------------------------------------------
	// Observers
	//--------------------------------------------------------------------
public:
	// If the observer is destroyed, be sure to remove it.
	void addObserver(LLInventoryObserver* observer);
	void removeObserver(LLInventoryObserver* observer);
	BOOL containsObserver(LLInventoryObserver* observer) const;
private:
	typedef std::set<LLInventoryObserver*> observer_list_t;
	observer_list_t mObservers;
	
/**                    Notifications
 **                                                                            **
 *******************************************************************************/


/********************************************************************************
 **                                                                            **
 **                    MISCELLANEOUS
 **/

	//--------------------------------------------------------------------
	// Callbacks
	//--------------------------------------------------------------------
public:
	// Trigger a notification and empty the folder type (FT_TRASH or FT_LOST_AND_FOUND) if confirmed
	void emptyFolderType(const std::string notification, LLFolderType::EType folder_type);
	bool callbackEmptyFolderType(const LLSD& notification, const LLSD& response, LLFolderType::EType preferred_type);
	static void registerCallbacks(LLMessageSystem* msg);

	//--------------------------------------------------------------------
	// File I/O
	//--------------------------------------------------------------------
protected:
	friend class LLLocalInventory;
	static bool loadFromFile(const std::string& filename,
							 cat_array_t& categories,
							 item_array_t& items,
							 bool& is_cache_obsolete); 
	static bool saveToFile(const std::string& filename,
						   const cat_array_t& categories,
						   const item_array_t& items); 

	//--------------------------------------------------------------------
	// Message handling functionality
	//--------------------------------------------------------------------
public:
	static void processUpdateCreateInventoryItem(LLMessageSystem* msg, void**);
	static void removeInventoryItem(LLUUID agent_id, LLMessageSystem* msg, const char* msg_label);
	static void processRemoveInventoryItem(LLMessageSystem* msg, void**);
	static void processUpdateInventoryFolder(LLMessageSystem* msg, void**);
	static void removeInventoryFolder(LLUUID agent_id, LLMessageSystem* msg);
	static void processRemoveInventoryFolder(LLMessageSystem* msg, void**);
	static void processRemoveInventoryObjects(LLMessageSystem* msg, void**);
	static void processSaveAssetIntoInventory(LLMessageSystem* msg, void**);
	static void processBulkUpdateInventory(LLMessageSystem* msg, void**);
	static void processInventoryDescendents(LLMessageSystem* msg, void**);
	static void processMoveInventoryItem(LLMessageSystem* msg, void**);
	static void processFetchInventoryReply(LLMessageSystem* msg, void**);
protected:
	bool messageUpdateCore(LLMessageSystem* msg, bool do_accounting);

	//--------------------------------------------------------------------
	// Locks
	//--------------------------------------------------------------------
public:
	void lockDirectDescendentArrays(const LLUUID& cat_id,
									cat_array_t*& categories,
									item_array_t*& items);
	void unlockDirectDescendentArrays(const LLUUID& cat_id);
protected:
	cat_array_t* getUnlockedCatArray(const LLUUID& id);
	item_array_t* getUnlockedItemArray(const LLUUID& id);
private:
	std::map<LLUUID, bool> mCategoryLock;
	std::map<LLUUID, bool> mItemLock;


public:
	// *NOTE: DEBUG functionality
	void dumpInventory() const;
	
/**                    Miscellaneous
 **                                                                            **
 *******************************************************************************/
};

// a special inventory model for the agent
extern LLInventoryModel gInventory;

#endif // LL_LLINVENTORYMODEL_H

