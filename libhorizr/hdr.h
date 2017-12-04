// static functions are s_funcname



#ifndef __MYMOD_H_INCLUDED__
#define __MYMOD_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _myp_myclass_t myp_myclass_t;

//  Create a new <class name> instance
CZMQ_EXPORT myp_myclass_t *
    myp_myclass_new (void);

//  Destroy a <class name> instance
CZMQ_EXPORT void
    myp_myclass_destroy (myp_myclass_t **self_p);

//  Self test of this class
void
    myp_myclass_test (bool verbose);

#ifdef __cplusplus
}
#endif

#endif

Here is a similar template header file for stateless classes:

#ifndef __MYMOD_H_INCLUDED__
#define __MYMOD_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Self test of this class
int
    myp_myclass_test (bool verbose);

#ifdef __cplusplus
}
#endif

#endif


/  Structure of our class

struct _myclass_t {
    <type> <name>;              //  <description>
};


//  Create a new myp_myclass instance
myp_myclass_t *
myp_myclass_new (<arguments>)
{
    myp_myclass_t *self = (myp_myclass_t *) zmalloc (sizeof (myp_myclass_t));
    assert (self);
    self->someprop = someprop_new ();
    assert (self->someprop);
    return self;
}

//  Create a new myp_myclass instance
myp_myclass_t *
myp_myclass_new (<arguments>)
{
    myp_myclass_t *self = (myp_myclass_t *) zmalloc (sizeof (myp_myclass_t));
    assert (self);
    self->someprop = someprop_new ();
    assert (self->someprop);
    return self;
}

//  Create a new myp_myclass instance
myp_myclass_t *
myp_myclass_new (<arguments>)
{
    myp_myclass_t *self = (myp_myclass_t *) zmalloc (sizeof (myp_myclass_t));
    assert (self);
    self->someprop = someprop_new ();
    assert (self->someprop);
    return self;
}

//  Create a new myp_myclass instance
myp_myclass_t *
myp_myclass_new (<arguments>)
{
    myp_myclass_t *self = (myp_myclass_t *) zmalloc (sizeof (myp_myclass_t));
    assert (self);
    self->someprop = someprop_new ();
    assert (self->someprop);
    return self;
}

//  Return first item in the list or null if the list is empty

item_t *
myp_myclass_first (myp_myclass_t *self)
{
    assert (self);
    //  Reset cursor to first item in list
    return item;
}

//  Return next item in the list or null if there are no more items

item_t *
myp_myclass_next (myp_myclass_t *self)
{
    assert (self);
    //  Move cursor to next item in list
    return item;
}

//  Return the value of myprop
<type>
myp_myclass_myprop (myp_myclass_t *self)
{
    assert (self);
    return self->myprop;
}
//  Set the value of myprop
void
myp_myclass_set_myprop (myp_myclass_t *self, <type> myprop)
{
    assert (self);
    self->myprop = myprop;
}


for (array_index = 0; array_index < array_size; array_index++) {
    //  Access element [array_index]
}
 
    The recommended pattern for list iteration is:

myp_myclass_t *myclass = (myp_myclass_t *) myp_myclass_first (myclass);
while (myclass) {
    //  Do something
    myclass = (myp_myclass_t *) myp_myclass_next (myclass);
}
