#ifndef _H_MLIST_
#define _H_MLIST_

#include <assert.h>

#define GET_ELE(vp) ((mlist_ele<T>*)(vp))

// make it explicit
#define FIRST_ITR 0
#define SECOND_ITR 1

template<class T>
struct mlist_ele
{
	mlist_ele():next(0),prev(0){}
	T val;
	mlist_ele<T>* next;
	mlist_ele<T>* prev;
};

// circular linked list
template<class T>
class mlist
{
public:
	mlist():head(0),tail(0),cnt(0){}
	~mlist(){clear();}

	T* operator[](int i)
	{
		if(i<0||i>=cnt) return 0;
		mlist_ele<T>* p=head;
		while(i--) p=p->next;
		return &(p->val);
	}

	void clear()
	{
		if(!cnt||!head||!tail) return;
		mlist_ele<T>* save;
		while(head)
		{
			save=head;
			head=head->next;
			delete save;
		}
		cnt=0;head=0;tail=0;
	}

	T* append()
	{
		mlist_ele<T>* ele=new mlist_ele<T>();
		if(++cnt==1)
		{
			head=tail=ele;
		}
		else
		{
			tail->next=ele;ele->prev=tail;
			tail=ele;
		}
		
		return &ele->val;
	}
	
	void remove(T* val)
	{
		if(!val||cnt<=0) return;		
		mlist_ele<T>* this_ele=GET_ELE(val);

		// assume in list !!!
		if(--cnt>0)
		{
			//assert((long)this_ele->next!=0xdddddddd&&(long)this_ele->prev!=0xdddddddd&&
			//	(long)this_ele->next!=0xfdfdfdfd&&(long)this_ele->prev!=0xfdfdfdfd);
			if(this_ele->prev) this_ele->prev->next=this_ele->next;
			else head=this_ele->next;
			if(this_ele->next) this_ele->next->prev=this_ele->prev;
			else tail=this_ele->prev;
		}
		else
		{
			head=tail=0;
		}
		delete this_ele;
	}

	int size(){return cnt;}
	
	T* begin(int i=FIRST_ITR,T* start=0)
	{
		mlist_ele<T>* s=(start?GET_ELE(start):head);
		return (itr[i]=s)?&(itr[i]->val):0;
	}
	T* next(int i=FIRST_ITR)
	{
		return (!itr[i])?0:(
			(!(itr[i]=itr[i]->next)/*||itr[i]==head*/)?
				0:&(itr[i]->val)
			);
	}

private:
	mlist_ele<T>* head,*tail,
		*itr[2]; // provide two iterators for O(n^2) traversal
	int cnt;
};

#endif